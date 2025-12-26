// File: protocol-ecros.cpp


//#include "../wellcoordination/benchmark/ecros-project.hpp"

#include "../tcp/config.hpp"

// ---------------------------------------------------------------------------
// Handler: owns the generic ECRO protocol state & algorithms
// ---------------------------------------------------------------------------

#ifdef STACK
    #include "../wellcoordination/benchmark/ecros-stack.hpp";
#endif

#ifdef SET
    #include "../wellcoordination/benchmark/ecros-set.hpp";
#endif

#ifdef PROJECT
    #include "../wellcoordination/benchmark/ecros-project.hpp";
#endif

#ifdef MOVIE
    #include "../wellcoordination/benchmark/ecros-movie.hpp";
#endif

#ifdef COURSEWARE
    #include "../wellcoordination/benchmark/ecros-courseware.hpp";
#endif

class Handler
{
public:
    // Pending local calls that could not proceed due to locked resources
    // std::map<int, Call> pendingLocalCalls;

    // Mutex to protect access to pendingLocalCalls
    // std::mutex pendMtx;
#ifdef STACK
    Stack obj; // Stack‐specific state & stubs live here
#endif

#ifdef SET
    Set obj; // Set‐specific state & stubs live here

#endif

#ifdef PROJECT
    Project obj; // Project‐specific state & stubs live here

#endif

#ifdef MOVIE
    Movie obj; // Movie‐specific state & stubs live here
#endif

#ifdef COURSEWARE
    Courseware obj; // Courseware‐specific state & stubs live here
#endif

    struct Edge
    {
        Call *from;
        Call *to;
        std::string type;
        // std::string label;
        bool operator==(const Edge &other) const
        {
            return from == other.from && to == other.to && type == other.type;
        }
    };
    std::map<int, int> ackCounter;
    int node_id;
    int number_of_nodes;
    std::mutex mtx;
    std::vector<int> seen; // Initialize to size = number_of_nodes with all 0s
    std::unordered_set<Call *> stableCalls;
    int expected_calls;
    std::atomic<bool> last_call_stable;
    int write_percentage; // Percentage of write operations (0-100)
    //std::vector<std::vector<int>> acks;

    // protocol‐wide state:
    std::vector<int>
        vc;                           // vector clock
    std::unordered_set<Call *> calls; // ECRO call‐set C
    std::vector<Edge> edges;          // execution graph E
    std::vector<Call *> topoOrder;    // topological order t
    std::map<std::pair<int, int>, int> ackCounts;

    // resource‐locking:
    // std::vector<std::mutex> resourceLocks;
    std::map<int, std::vector<int>> callLockMap;

    Handler()
        : node_id(0), number_of_nodes(0),
          vc()
    //, resourceLocks(1)
    {
        // seen.resize(number_of_nodes, 0);
    }

    void setVars(int id, int num_nodes, int expected, int wp)
    {
        write_percentage = wp;
        last_call_stable.store(false);
        expected_calls=expected;
        node_id = id;
        number_of_nodes = num_nodes;
        vc.assign(num_nodes, 0);
        // resourceLocks = std::vector<std::mutex>(1);
        obj.setVars(id, num_nodes);
        seen.resize(number_of_nodes, 0);
        //acks.resize(number_of_nodes, std::vector<int>(350000, 0));
    }

    static size_t serializeCalls(Call call, uint8_t *buffer)
    {
        std::string type = call.type;
        int value1 = call.value1;
        int value2 = call.value2;
        int node_id = call.node_id;
        int call_id = call.call_id;
        bool stable = call.stable;
        bool applied = call.applied; // NEW FIELD
        std::vector<int> call_vector_clock = call.call_vector_clock;

        uint8_t *start = buffer + sizeof(uint64_t);
        auto temp = start;

        uint64_t type_len = type.size();
        *reinterpret_cast<uint64_t *>(start) = type_len;
        start += sizeof(type_len);
        memcpy(start, type.c_str(), type_len);
        start += type_len;

        *reinterpret_cast<int *>(start) = value1;
        start += sizeof(value1);
        *reinterpret_cast<int *>(start) = value2;
        start += sizeof(value2);
        *reinterpret_cast<int *>(start) = node_id;
        start += sizeof(node_id);
        *reinterpret_cast<int *>(start) = call_id;
        start += sizeof(call_id);

        *reinterpret_cast<bool *>(start) = stable;
        start += sizeof(stable);
        *reinterpret_cast<bool *>(start) = applied; // serialize applied
        start += sizeof(applied);

        for (int vc : call_vector_clock)
        {
            *reinterpret_cast<int *>(start) = vc;
            start += sizeof(vc);
        }

        uint64_t len = start - temp;
        auto length = reinterpret_cast<uint64_t *>(start - len - sizeof(uint64_t));
        *length = len;

        // std::cout << "[Serialize] Call: type=" << type << ", node_id=" << node_id << ", call_id=" << call_id << std::endl;
        return len + sizeof(uint64_t);
    }
    void deserializeCalls(uint8_t *buffer, Call &call)
    {
        uint8_t *start = buffer + sizeof(uint64_t);

        uint64_t type_len = *reinterpret_cast<uint64_t *>(start);
        start += sizeof(type_len);
        std::string type(reinterpret_cast<char *>(start), type_len);
        start += type_len;

        int value1 = *reinterpret_cast<int *>(start);
        start += sizeof(value1);
        int value2 = *reinterpret_cast<int *>(start);
        start += sizeof(value2);
        int node_id = *reinterpret_cast<int *>(start);
        start += sizeof(node_id);
        int call_id = *reinterpret_cast<int *>(start);
        start += sizeof(call_id);

        bool stable = *reinterpret_cast<bool *>(start);
        start += sizeof(stable);
        bool applied = *reinterpret_cast<bool *>(start); // deserialize applied
        start += sizeof(applied);

        std::vector<int> call_vector_clock(number_of_nodes, 0);
        for (int &vc : call_vector_clock)
        {
            vc = *reinterpret_cast<int *>(start);
            start += sizeof(vc);
        }

        call = Call(type, value1, value2, node_id, call_id, stable);
        call.applied = applied; // assign applied value
        call.call_vector_clock = call_vector_clock;

        // std::cout << "[Deserialize] Call: type=" << type << ", node_id=" << node_id << ", call_id=" << call_id << std::endl;
    }

    // -----------------------------------------------------------------------
    // Local Invocation (fast‐path, but now re‐drives all in‐flight calls)
    // -----------------------------------------------------------------------
    void exeLocal(Call &call)
    {
        /*std::cout << "[exeLocal] type: " << call.type
                  << " call_id: " << call.call_id
                  << " node_id: " << call.node_id
                  << " value1: " << call.value1
                  << " calls size: " << calls.size() << std::endl;*/

        // Use-case specific skip logic (e.g., skip pop on empty stack)
        if (obj.shouldSkip(call))
        {
            /*std::cout << "[exeLocal] Skipping call due to use-case constraint - call id: "
                      << call.call_id << std::endl;*/
            return;
        }

        std::lock_guard<std::mutex> lk(mtx);

        vc[node_id]++;
        call.call_vector_clock = vc;
        call.node_id = node_id;

        Call *cptr = new Call(call);
        calls.insert(cptr);

        for (auto *v : calls)
        {
            if (v != cptr)
            {
                if (happenedBefore(v, cptr) && !seqCommutative(v, cptr))
                    edges.push_back({v, cptr, "hb"});
                if (happenedBefore(cptr, v) && !seqCommutative(cptr, v))
                    edges.push_back({cptr, v, "hb"});
            }
        }

        obj.applyCall(cptr);
        // commitStableCalls();
    }

    void localHandler(Call &call)
    {
        exeLocal(call);
    }

    // -----------------------------------------------------------------------
    // Remote Invocation (full‐replay, now for all in‐flight calls)
    // -----------------------------------------------------------------------
    /*
    void handleAckAndMaybeStabilize(Call &ack)
    {
        //std::cout << "[handleAckAndMaybeStabilize] we are in handleAckAndMaybeStabilize1 - call_id: " << ack.call_id << std::endl;

        // Safely update the seen vector clock (up to number_of_nodes)
        //cout<< "number_of_nodes: " << number_of_nodes<<"  seen size "<< seen.size() << "call_vector_clock size "<< ack.call_vector_clock.size()<<endl;
        for (int i = 0; i < number_of_nodes; ++i)
        {
            if (i < ack.call_vector_clock.size())
            {
                seen[i] = std::max(seen[i], ack.call_vector_clock[i]);
            }
            else
            {
                std::cerr << "[handleAckAndMaybeStabilize] Warning: ack.call_vector_clock size (" << ack.call_vector_clock.size()
                          << ") is smaller than expected number_of_nodes (" << number_of_nodes << ")" << std::endl;
            }
        }

        //std::cout << "[handleAckAndMaybeStabilize] we are in handleAckAndMaybeStabilize2 - updated seen: ";
        //for (int i = 0; i < number_of_nodes; ++i)
            //std::cout << seen[i] << " ";
        //std::cout << std::endl;

        // Try to mark calls as stable
        for (auto *c : calls)
        {
            if (c->stable)
                continue;

            bool allSeen = true;
            for (int i = 0; i < number_of_nodes; ++i)
            {
                if (seen[i] < c->call_vector_clock[i])
                {
                    allSeen = false;
                    break;
                }
            }

            if (allSeen)
            {
                c->stable = true;
                std::cout << "[handleAckAndMaybeStabilize] Stabel Marked call id " << c->call_id << " as stable via VC" << std::endl;
            }
        }

        //std::cout << "[handleAckAndMaybeStabilize] we are in handleAckAndMaybeStabilize3 - done checking stabilities" << std::endl;
    }*/

    /*void tryProcessPendingLocal()
    {
        std::lock_guard<std::mutex> plk(pendMtx);
        for (auto it = pendingLocalCalls.begin(); it != pendingLocalCalls.end();)
        {
            auto rs = restrictions(&(it->second));
            bool canLock = true;
            for (int r : rs)
            {
                if (!resourceLocks[r].try_lock())
                {
                    canLock = false;
                    break;
                }
            }
            if (canLock)
            {
                Call ready = it->second;
                callLockMap[ready.call_id] = rs;
                pendingLocalCalls.erase(it++);
                {
                    std::lock_guard<std::mutex> lk(mtx);
                    vc[node_id]++;
                    ready.call_vector_clock = vc;
                    ready.node_id = node_id;
                    Call *cptr = new Call(ready);
                    calls.insert(cptr);
                    for (auto *v : calls)
                        if (v != cptr)
                        {
                            if (happenedBefore(v, cptr) && !seqCommutative(v, cptr))
                                edges.push_back({v, cptr, "hb"});
                            if (happenedBefore(cptr, v) && !seqCommutative(cptr, v))
                                edges.push_back({cptr, v, "hb"});
                        }
                    obj.applyCall(cptr);
                    commitStableCalls();
                }
            }
            else
            {
                ++it; // move to next if we couldn't lock
            }
        }
    }*/
    void handleAckAndMaybeStabilize(Call &ack)
    {
        std::pair<int, int> key = {ack.node_id, ack.call_id};
        ackCounts[key]++;

        // Who is responsible for stabilizing this call?
        bool isSender = (ack.node_id == node_id);
        int required = isSender ? (number_of_nodes - 1) : (number_of_nodes - 2);

        if (ackCounts[key] >= required)
        {
            for (auto *c : calls)
            {
                if (c->node_id == ack.node_id && c->call_id == ack.call_id && !c->stable)
                {
                    c->stable = true;
                    if(c->node_id == node_id && c->call_id == (expected_calls/number_of_nodes) -1)
                        last_call_stable.store(true);
                    std::cout << "[AckStabilizer] Marked call_id=" << c->call_id
                              << " node=" << c->node_id << " as STABLE. Ack count="
                              << ackCounts[key] << " / " << required << "call.size() " << calls.size() << std::endl;
                    stableCalls.insert(c);
                    commitStableCalls(); // Safe to clean
                    obj.waittobestable.store(obj.waittobestable.load() + 1);
                    break; // Only one call should match
                }
            }
        }
    }

    void exeRemote(Call call)
    {
        std::lock_guard<std::mutex> lk(mtx);

        if (call.type == "Ack")
        {
            // std::cout << "Exe - by ack received - call id: " << call.call_id << std::endl;
            handleAckAndMaybeStabilize(call);
            // commitStableCalls();
            // tryProcessPendingLocal();
            return;
        }

        /*std::cout << "[exeRemote] type: " << call.type
                  << " call_id: " << call.call_id
                  << " node_id: " << call.node_id
                  << " value1: " << call.value1
                  << " calls size: " << calls.size() << std::endl;*/
        for (int i = 0; i < number_of_nodes; ++i)
            vc[i] = std::max(vc[i], call.call_vector_clock[i]);

        Call *newPtr = new Call(call);
        calls.insert(newPtr);

        // Optimize: compare only the new call with all existing (non-stable) calls
        for (auto *v : calls)
        {
            if (v == newPtr || v->stable)
                continue;

            if (happenedBefore(v, newPtr) && !seqCommutative(v, newPtr))
                edges.push_back({v, newPtr, "hb"});
            else if (happenedBefore(newPtr, v) && !seqCommutative(newPtr, v))
                edges.push_back({newPtr, v, "hb"});
            else if (concurrent(v, newPtr))
            {
                if(resolution(newPtr, v) == '<')
                {
                    edges.push_back({newPtr, v, "co"});
                }
                else if (resolution(newPtr, v) == '>')
                {
                    edges.push_back({v, newPtr, "co"});
                }
                else if (resolution(v, newPtr) == 'T' && !commutative(v, newPtr))
                {
                    char r = idcmp(newPtr, v);
                    if (r == '<')
                        edges.push_back({newPtr, v, "ao"});
                    else
                        edges.push_back({v, newPtr, "ao"});
                }
            }
        }

        detectAndResolveCycles(newPtr);

        fullTopoSort();
        recomputeState();
        // commitStableCalls();

        obj.send_ack = true;
        obj.send_ack_call.type = "Ack";
        obj.send_ack_call.node_id = call.node_id;
        obj.send_ack_call.call_id = call.call_id;
        obj.send_ack_call.value1 = call.value1;
        obj.send_ack_call.call_vector_clock = seen;

        auto old_list = std::atomic_load(&obj.send_ack_call_list);
        auto new_list = std::make_shared<std::vector<Call>>(*old_list);
        new_list->push_back(obj.send_ack_call);
        std::atomic_store(&obj.send_ack_call_list, new_list);

        obj.ackindex.fetch_add(1);
        // std::cout << "[ACK Handle] Updated ackindex to " << obj.ackindex.load() << std::endl;
    }

    void internalDownstreamExecute(Call call)
    {
        exeRemote(call);
    }

private:
    // -------------------- Generic ECRO Protocol Methods --------------------

    // vector-clock causality test
    bool happenedBefore(const Call *a, const Call *b)
    {
        auto &va = a->call_vector_clock;
        auto &vb = b->call_vector_clock;
        bool strictlyLess = false;
        for (size_t i = 0; i < number_of_nodes; ++i)
        {
            if (va[i] > vb[i])
                return false;
            if (va[i] < vb[i])
                strictlyLess = true;
        }
        return strictlyLess;
    }

    // Returns true if a and b are concurrent (i.e., neither happened before the other)
    bool concurrent(const Call *a, const Call *b)
    {
        auto &va = a->call_vector_clock;
        auto &vb = b->call_vector_clock;

        bool aLessThanB = false;
        bool bLessThanA = false;

        for (size_t i = 0; i < number_of_nodes; ++i)
        {
            if (va[i] < vb[i])
                aLessThanB = true;
            else if (vb[i] < va[i])
                bLessThanA = true;
        }

        // Concurrent if both a < b and b < a are false (neither happened before the other)
        return aLessThanB && bLessThanA;
    }

    void detectAndResolveCycles(Call *start)
    {
        // std::cout << "we are in detectAndResolveCycles start\n";

        std::unordered_set<Call *> visited;
        std::unordered_map<Call *, Call *> parent;
        std::stack<Call *> stack;
        std::unordered_map<Call *, std::string> incomingEdgeType;

        stack.push(start);

        while (!stack.empty())
        {
            Call *curr = stack.top();
            stack.pop();

            if (visited.count(curr))
                continue;
            visited.insert(curr);

            // std::cout << "we are in detectAndResolveCycles node " << curr->call_id << "\n";

            for (const auto &edge : edges)
            {
                if (edge.from == curr)
                {
                    Call *neighbor = edge.to;

                    if (!visited.count(neighbor))
                    {
                        parent[neighbor] = curr;
                        incomingEdgeType[neighbor] = edge.type;
                        stack.push(neighbor);
                    }
                    else if (parent[curr] != neighbor)
                    {
                        // std::cout << "we are in detectAndResolveCycles found cycle\n";

                        std::vector<Edge> cycle;
                        Call *p = curr;
                        while (p != neighbor && parent.count(p))
                        {
                            cycle.push_back({parent[p], p, incomingEdgeType[p]});
                            p = parent[p];
                        }

                        cycle.push_back({curr, neighbor, edge.type});

                        auto ao_it = std::find_if(cycle.begin(), cycle.end(), [](const Edge &e)
                                                  { return e.type == "ao"; });

                        if (ao_it != cycle.end())
                        {
                            // std::cout << "we are in detectAndResolveCycles breaking ao\n";
                            edges.erase(std::remove(edges.begin(), edges.end(), *ao_it), edges.end());
                            return;
                        }
                        else
                        {
                            // std::cout << "we are in detectAndResolveCycles fallback co\n";
                            Edge front = cycle.front();

                            auto tie_it = std::find_if(edges.begin(), edges.end(), [&](const Edge &e)
                                                       { return e.from == front.from &&
                                                                e.to == front.to &&
                                                                e.type == front.type; });

                            if (tie_it != edges.end())
                            {
                                Call *from_ptr = tie_it->from;
                                Call *to_ptr = tie_it->to;

                                edges.erase(tie_it);
                                edges.push_back({from_ptr, to_ptr, "co"});
                            }
                            return;
                        }
                    }
                }
            }
        }

        // std::cout << "we are in detectAndResolveCycles end\n";
    }

    // full topological sort (Kahn’s algorithm)
    void fullTopoSort()
    {
        std::map<Call *, int> indeg;
        for (auto *u : calls)
            indeg[u] = 0;
        for (auto &e : edges)
            indeg[e.to]++;
        std::vector<Call *> q;
        for (auto &kv : indeg)
            if (kv.second == 0)
                q.push_back(kv.first);

        topoOrder.clear();
        while (!q.empty())
        {
            Call *u = q.back();
            q.pop_back();
            topoOrder.push_back(u);
            for (auto &e : edges)
                if (e.from == u)
                    if (--indeg[e.to] == 0)
                        q.push_back(e.to);
        }
    }

    // replay using Stack::recomputeState
    void recomputeState()
    {
        obj.recomputeState(topoOrder);
    }

    void commitStableCalls()
    {
        for (auto *c : stableCalls)
        {
            calls.erase(c); // O(log n)
            edges.erase(std::remove_if(edges.begin(), edges.end(),
                                       [c](const Edge &e)
                                       {
                                           return e.from == c || e.to == c;
                                       }),
                        edges.end());
            delete c;
        }
        stableCalls.clear();
    }
};
