#include "../wellcoordination/benchmark/project.hpp"
#include "../wellcoordination/benchmark/stack.hpp"
#include "../wellcoordination/benchmark/set.hpp"
#include "../wellcoordination/benchmark/movie.hpp"
#include "../wellcoordination/benchmark/courseware.hpp"

#include "../wellcoordination/benchmark/op-crdt-reg.hpp"
#include "../wellcoordination/benchmark/op-crdt-gset.hpp"
#include "../wellcoordination/benchmark/op-crdt-counter.hpp"

#define FAILURE_MODE

// #define PROJECT
#define STACK
// #define SET
// #define MOVIE
// #define COURSEWARE


// #define CRDT // we need this if we want to run CRDTs Use Cases. Then need to select one of following options.
// #define OPCRDTCOUNTER
//  #define OPCRDTGSET
//  #define OPCRDTREG

// Define the Handler class
class Handler
{
public:
#ifdef PROJECT
    Project obj;
#endif

#ifdef MOVIE
    Movie obj;
#endif

#ifdef COURSEWARE
    Courseware obj;
#endif

#ifdef STACK
    Stack obj;
#endif

#ifdef SET
    Set obj;
#endif

#ifdef OPCRDTCOUNTER
    Opcrdtcounter obj;
#endif

#ifdef OPCRDTGSET
    Opcrdtgset obj;
#endif

#ifdef OPCRDTREG
    Opcrdtreg obj;
#endif

    std::mutex mtx;
    std::mutex mtx_ack;
    std::vector<Call> executionList;
    int node_id;
    int number_of_nodes;
    int expected_calls;
    int write_percentage;
    std::vector<int> vector_clock;
    std::vector<std::vector<int>> remote_verctor_clocks;
    std::atomic<bool> last_call_stable;
    std::vector<std::vector<int>> acks;

    int failed[64];

    // std::vector<Call> queuedList;
    int test;
    std::priority_queue<Call, std::vector<Call>, std::function<bool(const Call &, const Call &)>> priorityQueue;

    // Default constructor
    Handler() : vector_clock(), remote_verctor_clocks(),
                priorityQueue([this](const Call &a, const Call &b)
                              { return this->obj.compareVectorClocks(a.call_vector_clock, b.call_vector_clock) > 0; }) {}

public:
    void setVars(int id, int num_nodes, int expected, int wp)
    {
        write_percentage = wp;
        last_call_stable.store(false);
        expected_calls = expected;
        node_id = id;
        number_of_nodes = num_nodes;
        vector_clock.resize(num_nodes, 0);
        remote_verctor_clocks.resize(num_nodes, std::vector<int>(num_nodes, 0));
        acks.resize(num_nodes, std::vector<int>(350000, 0));
        obj.num_nodes = num_nodes;
        test = 0;
        for (int i = 0; i < num_nodes; ++i)
            failed[i]=false;
    }
    
    size_t serializeCalls(Call call, uint8_t *buffer)
    {
        std::string type = call.type;
        int value1 = call.value1;
        int value2 = call.value2;
        test++;
        int node_id = call.node_id;
        int call_id = call.call_id;
        bool stable = call.stable;
        std::vector<int> call_vector_clock = call.call_vector_clock;

        uint8_t *start = buffer + sizeof(uint64_t);
        auto temp = start;

        // Serialize type
        uint64_t type_len = type.size();
        *reinterpret_cast<uint64_t *>(start) = type_len;
        start += sizeof(type_len);
        memcpy(start, type.c_str(), type_len);
        start += type_len;

        // Serialize value1
        *reinterpret_cast<int *>(start) = value1;
        start += sizeof(value1);

        // Serialize value2
        *reinterpret_cast<int *>(start) = value2;
        start += sizeof(value2);

        // Serialize node_id
        *reinterpret_cast<int *>(start) = node_id;
        start += sizeof(node_id);

        // Serialize call_id
        *reinterpret_cast<int *>(start) = call_id;
        start += sizeof(call_id);

        // Serialize stable
        *reinterpret_cast<bool *>(start) = stable;
        start += sizeof(stable);

        // Serialize call_vector_clock
        for (int vc : call_vector_clock)
        {
            *reinterpret_cast<int *>(start) = vc;
            start += sizeof(vc);
        }

        uint64_t len = start - temp;

        auto length = reinterpret_cast<uint64_t *>(start - len - sizeof(uint64_t));
        *length = len;

        return len + sizeof(uint64_t) + 2 * sizeof(uint64_t);
    }
    
    // MODIFIED: Now triggers re-evaluation
    void setfailurenode(int id)
    {
        failed[id - 1] = true;
        cout << "[FAILURE_HANDLING] Node " << id << " marked as failed" << endl;
        
        // NEW: Re-evaluate all pending operations with new quorum
        reEvaluateStabilityAfterFailure(id);
    }
    
    // NEW FUNCTION: Re-evaluate stability after a node failure
    void reEvaluateStabilityAfterFailure(int failed_node_id)
    {
        std::lock_guard<std::mutex> lock(mtx);
        
        std::cout << "[FAILURE_HANDLING] Re-evaluating stability after node " 
                  << failed_node_id << " failure" << std::endl;
        
        // Recalculate quorum
        int quorum = number_of_nodes - 1;
        int num_failed = 0;
        for (int i = 0; i < number_of_nodes; ++i)
            if (failed[i])
                num_failed++;
        quorum -= num_failed;
        
        std::cout << "[FAILURE_HANDLING] New quorum: " << quorum 
                  << " (failed nodes: " << num_failed << ")" << std::endl;
        
        // Re-check all unstable operations with new quorum
        int i = obj.stable_state.index;
        bool found_new_stable = false;
        
        while (i < executionList.size())
        {
            bool now_stable = false;
            
            // Skip operations from failed nodes
            if (failed[executionList[i].node_id - 1])
            {
                std::cout << "[FAILURE_HANDLING] Skipping operation from failed node " 
                          << executionList[i].node_id << std::endl;
                i++;
                continue;
            }
            
            if (executionList[i].node_id == node_id)
            {
                if (acks[executionList[i].node_id - 1][executionList[i].call_id] >= quorum)
                {
                    now_stable = true;
                }
            }
            else
            {
                // For remote operations, check if they have enough acks
                if (acks[executionList[i].node_id - 1][executionList[i].call_id] >= (quorum - 1))
                {
                    now_stable = true;
                }
            }
            
            if (now_stable)
            {
                std::cout << "[FAILURE_HANDLING] Operation " << i 
                          << " (from node " << executionList[i].node_id 
                          << ", call_id " << executionList[i].call_id
                          << ") now stable with adjusted quorum" << std::endl;
                found_new_stable = true;
                i++;
            }
            else
            {
                break;  // Stop at first unstable operation
            }
        }
        
        if (found_new_stable)
        {
            std::cout << "[FAILURE_HANDLING] Found operations that can now stabilize, triggering stabilization" << std::endl;
            // Trigger normal stabilization process
            stabilizerWithAck();
        }
    }
    
    void deserializeCalls(uint8_t *buffer, Call &call)
    {
        uint8_t *start = buffer + sizeof(uint64_t);
        auto temp = start;

        // Deserialize type
        uint64_t type_len = *reinterpret_cast<uint64_t *>(start);
        start += sizeof(type_len);
        std::string type(reinterpret_cast<char *>(start), type_len);
        start += type_len;

        // Deserialize value1
        int value1 = *reinterpret_cast<int *>(start);
        start += sizeof(value1);

        // Deserialize value2
        int value2 = *reinterpret_cast<int *>(start);
        start += sizeof(value2);

        // Deserialize node_id
        int node_id = *reinterpret_cast<int *>(start);
        start += sizeof(node_id);

        // Deserialize call_id
        int call_id = *reinterpret_cast<int *>(start);
        start += sizeof(call_id);

        // Deserialize stable
        bool stable = *reinterpret_cast<bool *>(start);
        start += sizeof(stable);

        // Deserialize call_vector_clock
        std::vector<int> call_vector_clock(number_of_nodes, 0);
        for (int &vc : call_vector_clock)
        {
            vc = *reinterpret_cast<int *>(start);
            start += sizeof(vc);
        }

        // Assign the deserialized values to the call object
        call = Call(type, value1, value2, node_id, call_id, stable);
        call.call_vector_clock = call_vector_clock;
    }
    
    void localHandler(Call &call, bool &flag, bool &permiss, int &stableindex)
    {
        std::lock_guard<std::mutex> lock(mtx);
        stableindex = obj.stable_state.index;
        flag = false;
        permiss = false;
        if (obj.locallyPermissibility(call))
        {
            flag = true;
            permiss = true;
#ifndef CRDT
            for (int i = obj.stable_state.index; i < executionList.size(); ++i)
            {
                if (i >= executionList.size())
                {
                    std::cerr << "Index out of bounds: localHandler" << std::endl;
                    return;
                }
                if (obj.fastisreachablelocal(call, executionList[i]))
                    flag = false;
                if (!flag)
                    break;
            }
#endif
            if (flag)
            {
                vector_clock[node_id - 1]++;
                for (int i = 0; i < number_of_nodes; i++)
                {
                    if (i >= number_of_nodes || i >= vector_clock.size())
                    {
                        std::cerr << "Index out of bounds: localHandler" << std::endl;
                        return;
                    }
                    call.call_vector_clock.resize(number_of_nodes, 0);
                    call.call_vector_clock[i] = vector_clock[i];
                }
                executionList.push_back(call);

                obj.updateCurrentState(call);
                std::cout << "Exe - by local - type: " << call.type << " and value1: " << call.value1 << " Vector-Clocks: " << vector_clock[0] << "-" << vector_clock[1] << "-" << vector_clock[2] << " -call id -" << call.call_id << " -current index " << obj.current_state.index << std::endl;
                obj.current_state.index++;
#ifdef CRDT
                obj.stable_state.index++;
                obj.waittobestable.store(obj.waittobestable.load() + 1);
#endif
            }
        }
    }

    int findPosition(const std::string &type, const std::vector<Call> &calls)
    {
        for (int i = 0; i < calls.size(); ++i)
        {
            if (calls[i].type == type)
            {
                return i;
            }
        }
        return -1;
    }

    bool queueHandler(bool addorRemove, Call remoteCall)
    {
        bool flag = false;

        for (int i = 0; i < priorityQueue.size(); i++)
        {
            if (obj.compareVectorClocks(remoteCall.call_vector_clock, priorityQueue.top().call_vector_clock) == 2)
            {
                if (addorRemove)
                {
                    priorityQueue.push(remoteCall);
                }
                return true;
            }
        }
        std::lock_guard<std::mutex> lock(mtx);
        for (int i = obj.stable_state.index; i < executionList.size(); i++)
        {
            if (obj.fastisreachableremote(remoteCall, executionList[i]))
            {
                if (obj.compareVectorClocks(remoteCall.call_vector_clock, executionList[i].call_vector_clock) == 2)
                {
                    if (addorRemove)
                    {
                        priorityQueue.push(remoteCall);
                    }
                    return true;
                }
            }
        }

        return false;
    }

    bool remoteHandler(bool addorRemove, Call call)
    {
#ifndef CRDT
        bool add_queue_flag = false;
        add_queue_flag = queueHandler(addorRemove, call);
        if (!add_queue_flag)
        {
            {
                std::lock_guard<std::mutex> lock(mtx);
                auto pos = std::next(executionList.begin(), obj.stable_state.index);
                auto end = executionList.end();
                bool reachable = false;
                while (pos != end)
                {
                    reachable = obj.fastisreachableremote(call, *pos);
                    if (reachable)
                    {
                        break;
                    }
                    ++pos;
                }
                auto index = std::distance(executionList.begin(), pos);
                executionList.insert(pos, call);
                obj.current_state.index++;
                obj.updateCurrentState(call);
                vector_clock[call.node_id - 1]++;
            }
            obj.send_ack = true;
            obj.send_ack_call.type = "Ack";
            obj.send_ack_call.node_id = call.node_id;
            obj.send_ack_call.call_id = call.call_id;
            obj.send_ack_call.value1 = call.value1;

            auto new_ack_list = std::make_shared<std::vector<Call>>(*obj.send_ack_call_list);
            new_ack_list->push_back(obj.send_ack_call);
            std::atomic_store(&obj.send_ack_call_list, new_ack_list);
            obj.ackindex.store(obj.ackindex.load() + 1);

            return true;
        }
#endif
#ifdef CRDT
        auto pos = std::next(executionList.begin(), obj.stable_state.index);
        auto end = executionList.end();
        auto index = std::distance(executionList.begin(), pos);
        executionList.insert(pos, call);
        obj.current_state.index++;
        obj.stable_state.index++;
        obj.updateCurrentState(call);
        vector_clock[call.node_id - 1]++;
        obj.waittobestable.store(obj.waittobestable.load() + 1);
        std::cout << "Exe - by remote - type: " << call.type << " and value1: " << call.value1 << " Vector-Clocks: " << vector_clock[0] << "-" << vector_clock[1] << "-" << vector_clock[2] << " -call id -" << call.call_id << " -current index " << obj.current_state.index << " pos " << index << " size " << executionList.size() << std::endl;
        return true;
#endif
        return false;
    }

    void updateAcksTable(Call call)
    {
        int quorum = number_of_nodes - 1;
        acks[call.node_id - 1][call.call_id]++;

#ifdef FAILURE_MODE
        int num_failed = 0;
        for (int i = 0; i < number_of_nodes; ++i)
            if (failed[i])
                num_failed++;
        // cout << "updateAcksTable: num failed nodes: " << num_failed << std::endl;
        quorum -= num_failed;
#endif
        if (call.node_id == node_id)
        { 
            // cout << "acks[call.node_id - 1][call.call_id] is: " << acks[call.node_id - 1][call.call_id] << ", quorum is: " << quorum << std::endl; 
            if (acks[call.node_id - 1][call.call_id] == (quorum))
            {
                stabilizerWithAck();
            }
        }
        else
        {
            // cout << "acks[call.node_id - 1][call.call_id] is: " << acks[call.node_id - 1][call.call_id] << ", quorum-1 is: " << (quorum-1) << std::endl; 

            if (acks[call.node_id - 1][call.call_id] == (quorum - 1))
            {
                stabilizerWithAck();
            }
        }
    }
    
    // MODIFIED: Enhanced to handle failed nodes
    void stabilizerWithAck()
    {
        bool can_unqued = false;
        int quorum = number_of_nodes - 1;
#ifdef FAILURE_MODE
        int num_failed = 0;
        for (int i = 0; i < number_of_nodes; ++i)
            if (failed[i])
                num_failed++;

        // cout << "stabilizerWithAck: num failed nodes: " << num_failed << std::endl;

        quorum -= num_failed;
#endif
        {
            std::lock_guard<std::mutex> lock(mtx);
            int i = obj.stable_state.index;
            
            if (i >= executionList.size())
            {
                return;
            }

            bool stable = true;

            while (stable && i < executionList.size())
            {
                stable = false;
                
                // NEW: Skip operations from failed nodes that will never stabilize
#ifdef FAILURE_MODE
                if (failed[executionList[i].node_id - 1])
                {
                    std::cout << "[FAILURE_HANDLING] Auto-stabilizing operation from failed node " 
                              << executionList[i].node_id << std::endl;
                    i++;
                    obj.stable_state.index++;
                    obj.waittobestable.store(obj.waittobestable.load() + 1);
                    stable = true;  // Continue to next operation
                    continue;
                }
#endif
                
                if (executionList[i].node_id == node_id)
                {
                    if (acks[executionList[i].node_id - 1][executionList[i].call_id] >= quorum)
                    {
                        stable = true;
                        can_unqued = true;
                    }
                }
                else
                {
                    if (acks[executionList[i].node_id - 1][executionList[i].call_id] >= (quorum - 1))
                    {
                        stable = true;
                        can_unqued = true;
                    }
                }
                
                if (stable)
                {
                    // Check if this is the last call for this node
#ifdef FAILURE_MODE
                    int expected_local_calls = expected_calls / number_of_nodes;
                    if (executionList[i].node_id == node_id && 
                        executionList[i].call_id == expected_local_calls - 1)
                        last_call_stable.store(true);
#else
                    if (executionList[i].node_id == node_id && 
                        executionList[i].call_id == (expected_calls / number_of_nodes) - 1)
                        last_call_stable.store(true);
#endif
                    
                    std::cout << "Exe - stablized - type: " << executionList[i].type 
                              << " and value1: " << executionList[i].value1 
                              << " -stable index " << obj.stable_state.index
                              << " que size " << priorityQueue.size() << std::endl;
                    obj.updateStableState(executionList[i]);
                    i++;
                    obj.stable_state.index++;
                    obj.waittobestable.store(obj.waittobestable.load() + 1);
                }
            }
        }
        if (can_unqued)
        {
            while (!priorityQueue.empty())
            {
                Call topCall = priorityQueue.top();
                bool remove_flag = remoteHandler(false, topCall);
                if (remove_flag)
                {
                    priorityQueue.pop();
                }
                else
                {
                    break;
                }
            }
        }
    }

    int findCallIndex(const std::string &type, const std::vector<Call> &calls)
    {
        for (int i = 0; i < calls.size(); ++i)
        {
            if (i >= calls.size())
            {
                std::cerr << "Index out of bounds: findCallIndex" << std::endl;
                return -1;
            }
            if (calls[i].type == type)
            {
                return i;
            }
        }
        return -1;
    }

    bool checkOrder(const std::string &type1, const std::string &type2, const std::vector<Call> &calls)
    {
        int order1, order2;
        order1 = findCallIndex(type1, calls);
        order2 = findCallIndex(type2, calls);
        if (order2 <= order1)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    void internalDownstreamExecute(Call call)
    {
        remoteHandler(true, call);
#ifndef CRDT
        while (!priorityQueue.empty())
        {
            Call topCall = priorityQueue.top();
            bool remove_flag = remoteHandler(false, topCall);
            if (remove_flag)
            {
                priorityQueue.pop();
            }
            else
            {
                break;
            }
        }
#endif
    }
};