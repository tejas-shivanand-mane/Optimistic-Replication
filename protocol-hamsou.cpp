#include "../wellcoordination/benchmark/project.hpp"
#include "../wellcoordination/benchmark/stack.hpp"
#include "../wellcoordination/benchmark/set.hpp"
#include "../wellcoordination/benchmark/movie.hpp"
#include "../wellcoordination/benchmark/courseware.hpp"

#include "../wellcoordination/benchmark/op-crdt-reg.hpp"
#include "../wellcoordination/benchmark/op-crdt-gset.hpp"
#include "../wellcoordination/benchmark/op-crdt-counter.hpp"

// #define FAILURE_MODE

#define PROJECT
// #define STACK
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
public:

    std::atomic<bool> failure_just_handled{false};

    std::mutex mtx;
    std::mutex mtx_ack;
    
    std::mutex mtx_failure; 


    std::vector<Call> executionList;
    int node_id;
    int number_of_nodes;
    std::atomic<int> quorum;
    int last_failed_count;
    int failed_count;

    int expected_calls;
    int write_percentage;
    std::vector<int> vector_clock;
    std::vector<std::vector<int>> remote_verctor_clocks;
    std::atomic<bool> last_call_stable;
    std::vector<std::vector<int>> acks;

    int failed[64];
    std::unordered_set<int> failed_nodes;
    std::queue<int> pending_failures;
    std::mutex failure_queue_mutex;

    // std::vector<Call> queuedList;
    int test;
    std::priority_queue<Call, std::vector<Call>, std::function<bool(const Call &, const Call &)>> priorityQueue;

    // Default constructor
    Handler() : vector_clock(), remote_verctor_clocks(),
                priorityQueue([this](const Call &a, const Call &b)
                              { return this->obj.compareVectorClocks(a.call_vector_clock, b.call_vector_clock) > 0; }) {}


    void setVars(int id, int num_nodes, int expected, int wp)
    {
        write_percentage = wp;
        last_call_stable.store(false);
        expected_calls = expected;
        node_id = id;
        number_of_nodes = num_nodes;
        quorum = number_of_nodes - 1;

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
        // int value1 = test;
        // int value2 = test;
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

        // Debug print to verify serialized buffer
        // std::cout << "Serialized buffer: ";
        // for (size_t i = 0; i < len; ++i)
        //{
        // std::cout << std::hex << static_cast<int>(buffer[i]) << " ";
        //}
        // std::cout << std::endl;

        return len + sizeof(uint64_t) + 2 * sizeof(uint64_t);
    }

    void setfailurenode(int id)
    {
        std::cout << "Node " << id << " marked as failed" << std::endl;
        
        std::lock_guard<std::mutex> lock(mtx_failure);
        
        if (failed[id - 1]) {
            return;
        }
        
        failed[id - 1] = true;
        
        if (failed_nodes.count(id) == 0) {
            failed_nodes.insert(id);


            int old_quorum = quorum.fetch_sub(1);
            std::cout << "Quorum adjusted: " << old_quorum << " -> " << (old_quorum - 1) << std::endl;


            // Give all EXISTING operations a fake ack from the failed node
            {
                std::lock_guard<std::mutex> ack_lock(mtx);
                std::cout << "Adding fake acks from node " << id << " to all operations" << std::endl;
                
                for (int node = 0; node < number_of_nodes; node++) {
                    for (int call_id = 0; call_id < acks[node].size(); call_id++) {
                        if (acks[node][call_id] > 0 && acks[node][call_id] < old_quorum) {
                            // This operation exists and might benefit from an extra ack
                            acks[node][call_id]++;
                        }
                    }
                }
            }
    
            
            // This will set acks to 1, stabilize, AND remove unstabilized operations
            stabilizerWithAck();
            std::cout << "stabilizerWithAck() complete" << std::endl;
        }
    }

    void deserializeCalls(uint8_t *buffer, Call &call)
    {
        // std::cout << "deserializing calls" << std::endl;

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

        // Debug print to verify deserialized call_vector_clock
        // std::cout << "Deserialized call_vector_clock: ";
        // for (int vc : call_vector_clock)
        //{
        // std::cout << vc << " ";
        //}
        // std::cout << std::endl;

        // Assign the deserialized values to the call object
        call = Call(type, value1, value2, node_id, call_id, stable);
        call.call_vector_clock = call_vector_clock;

        // std::cout << "deserialized calls" << std::endl;

    }
    void localHandler(Call &call, bool &flag, bool &permiss, int &stableindex)
    {
        std::cout << "localHandler: start" << std::endl;

        std::lock_guard<std::mutex> lock(mtx);
        stableindex = obj.stable_state.index;
        flag = false;
        permiss = false;

        std::cout << "localHandler: after lock" << std::endl;

        if (obj.locallyPermissibility(call))
        {
            std::cout << "localHandler: locallyPermissibility loop" << std::endl;

            flag = true;
            permiss = true;
#ifndef CRDT
            for (int i = obj.stable_state.index; i < executionList.size(); ++i)
            {
                if (i >= executionList.size())
                {
                    std::cerr << "Index out of bounds: localHandler" << std::endl;
                    return; // Handle out-of-bounds access
                }
                // flag = checkOrder(call.type, executionList[i].type, obj.topologicalOrder);
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
                        return; // Handle out-of-bounds access
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

        std::cout << "localHandler: end" << std::endl;

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
        return -1; // Return -1 if no match is found
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
                    // std::cout << "Exe - queued1 - type: " << remoteCall.type << " and value1: " << remoteCall.value1 << std::endl;
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
                        std::cout << "Exe - queued2 - type: " << remoteCall.type << " and value1: " << remoteCall.value1 << std::endl;
                    }
                    return true;
                }
            }
        }

        return false;
    }

    bool remoteHandler(bool addorRemove, Call call)
    {

        if (failed[call.node_id - 1]) {
            std::cout << "Rejecting operation from failed node " << call.node_id 
                    << ": " << call.type << " (call_id=" << call.call_id << ")" << std::endl;
            return false;  // Don't add to execution list
        }



#ifndef CRDT
        bool add_queue_flag = false;
        std::cout << "Exe - by remote1 - type: " << call.type << " and value1: " << call.value1 << " Vector-Clocks: " << vector_clock[0] << "-" << vector_clock[1] << "-" << vector_clock[2] << vector_clock[3]<< " -call id -" << call.call_id << " -current index " << obj.current_state.index << " size " << executionList.size() << "bool "<<add_queue_flag<< "node id"<< call.node_id<< std::endl;
        add_queue_flag = queueHandler(addorRemove, call);
        // cout<< "check1"<<std::endl;
        if (!add_queue_flag)
        {
            {
                // cout<< "check2"<<std::endl;
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
                // std::cout << "Exe - by remote2 - type: " << call.type << " and value1: " << call.value1 << " Vector-Clocks: " << vector_clock[0] << "-" << vector_clock[1] << "-" << vector_clock[2] << vector_clock[3]<< " -call id -" << call.call_id << " -current index " << obj.current_state.index << " pos " << index << " size " << executionList.size() << std::endl;
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

    /*
        void stabilizerWithVC()
        {
            // std::lock_guard<std::mutex> lock(mtx);
            bool stable_flag = true;
            int i = obj.stable_state.index;
            // for (int i = tmp_stable_state_index; i < executionList.size(); ++i) // Eric, in execution list from stabe state index, we should check call vector clock to be less than all the last update of remote processes vector clock
            //{
            if (i >= executionList.size())
                std::cerr << "Index out of bounds: executionList" << std::endl;
            return;
            // then consider it stabe. it it is new stable state then chek remote handler on qued calls.
            for (int j = 0; j < number_of_nodes; j++)
            {

                if (j != node_id - 1)
                { // just check it for remote nodes
                    for (int k = 0; k < number_of_nodes; k++)
                    {
                        if (executionList[i].call_vector_clock[k] > remote_verctor_clocks[j][k])
                        {
                            stable_flag = false;
                            break;
                        }
                    }
                }
                if (stable_flag == false)
                    break;
            }
            if (stable_flag)
            {
                // std::lock_guard<std::mutex> lock(mtx);                // now consider this call as stable
                obj.stable_state.index++; // todo update sets and otherthings
                obj.waittobestable.store(obj.waittobestable.load() + 1);
                // std::cout << "stable index " << obj.stable_state.index<< std::endl;
                std::cout << "Exe - stablized - type: " << executionList[i].type << " and value1: " << executionList[i].value1 << " -stable index " << obj.stable_state.index << std::endl;
                obj.updateStableState(executionList[i]);
                if (!queuedList.empty())
                {
                    bool remove_flag;
                    remove_flag = remoteHandler(false, queuedList.front());
                    if (remove_flag)
                        queuedList.erase(queuedList.begin());
                }
            }
            //}
        }*/
    void updateAcksTable(Call call)
    {
        int current_quorum = quorum.load();


        cout<< "DEBUG updateAcksTable acks.size(), call.call_id: " << acks.size() << ", " << call.call_id <<  endl;


        // acks[call.node_id - 1][call.call_id]++;

        if (call.node_id <= 0 || call.node_id > number_of_nodes) return;

        auto &row = acks[call.node_id - 1];
        if (call.call_id >= (int)row.size()) row.resize(call.call_id + 1, 0);
        row[call.call_id]++;


// std::cout << "Exe - by update ack - type: " << call.type << " and value1: " << " -call id -" << call.call_id << "acks -- " << acks[call.node_id - 1][call.call_id] << "qu size  "<< priorityQueue.size()<<std::endl;
// #ifdef FAILURE_MODE
//         int num_failed = 0;
//         for (int i = 0; i < number_of_nodes; ++i)
//             if (failed[i])
//                 num_failed++;
//         cout<< "updateAcksTable: num failed nodes: " << num_failed << std::endl;
//         quorum -= num_failed;
// #endif
        if (call.node_id == node_id) {

            // cout << "acks[" << call.node_id << "][" << call.call_id << "] = " 
            //     << acks[call.node_id - 1][call.call_id] 
            //     << ", current_quorum = " << current_quorum << std::endl;
            
            if (acks[call.node_id - 1][call.call_id] >= current_quorum) {
                stabilizerWithAck();
            }
        } else {


            // cout << "acks[" << call.node_id << "][" << call.call_id << "] = " 
            //     << acks[call.node_id - 1][call.call_id] 
            //     << ", need " << (current_quorum - 1) 
            //     << " (current_quorum=" << current_quorum << ")" << std::endl;
            
            if (acks[call.node_id - 1][call.call_id] >= (current_quorum - 1)) {
                stabilizerWithAck();
            }
        }

    }



    void stabilizerWithAck()
    {


        bool can_unqued = false;
        
        {
            std::lock_guard<std::mutex> lock(mtx);
            int i = obj.stable_state.index;
            // std::lock_guard<std::mutex> lock(mtx_ack);
            if (i >= executionList.size())
            {
                return;
            }

            bool stable = true;





            std::cout << "i, executionList.size(): " << i << ", " << executionList.size() << std::endl;
            while (stable && i < executionList.size())
            {

                stable = false;
                if (executionList[i].node_id == node_id)
                {
                    if (acks[executionList[i].node_id - 1][executionList[i].call_id] >= (quorum))
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
                    if (executionList[i].node_id == node_id && executionList[i].call_id == (expected_calls / number_of_nodes) - 1)
                        last_call_stable.store(true);
                    std::cout << "Exe - stablized - type: " << executionList[i].type << " and value1: " << executionList[i].value1 << " -stable index " << obj.stable_state.index<< "que size"<< priorityQueue.size()<< std::endl;
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
                return -1; // Handle out-of-bounds access
            }
            if (calls[i].type == type)
            {
                return i;
            }
        }
        return -1; // Return -1 if no match is found
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

    // void internalExecute(Call &call)
    //{

    // localHandler(call);
    //}

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