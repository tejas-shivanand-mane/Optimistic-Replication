#include "../wellcoordination/benchmark/account.hpp"
#include "../wellcoordination/benchmark/project.hpp"
#define PROJECTUSECASE
// #define ACCOUNTUSECASE

// Define the Handler class
class Handler
{
public:
#ifdef PROJECTUSECASE
    Project obj;
#endif

#ifdef ACCOUNTUSECASE
    Account obj;
#endif

    std::mutex mtx;
    std::mutex mtx_ack;
    std::vector<Call> executionList;
    int node_id;
    int number_of_nodes;
    std::vector<int> vector_clock;
    std::vector<std::vector<int>> remote_verctor_clocks;
    std::vector<Call> queuedList;

    // Default constructor
    Handler() : vector_clock(8, 0), remote_verctor_clocks(8, std::vector<int>(8, 0)) {}

public:
    void setVars(int id, int num_nodes)
    {
        node_id = id;
        number_of_nodes = num_nodes;
    }
    int compareVectorClocks(std::vector<int> vc1, std::vector<int> vc2)
    {
        bool flag = false;

        for (int i = 0; i < vc1.size(); i++)
        {
            if (i >= vc1.size() || i >= vc2.size())
            {
                std::cerr << "Index out of bounds: compareVectorClocks" << std::endl;
                return -1; // Handle out-of-bounds access
            }
            if (vc1[i] > vc2[i])
            {
                flag = true;
            }
        }
        if (!flag)
        {
            return 0;
        }

        flag = true;
        for (int i = 0; i < vc1.size(); i++)
        {
            if (i >= vc1.size() || i >= vc2.size())
            {
                std::cerr << "Index out of bounds: compareVectorClocks" << std::endl;
                return -1; // Handle out-of-bounds access
            }
            if (vc1[i] < vc2[i])
            {
                flag = false;
            }
        }

        if (flag)
        {
            return 2;
        }
        return 1;
    }

    void localHandler(Call &call, bool &flag, bool &permiss, int &stableindex)
    {
        std::lock_guard<std::mutex> lock(mtx);
        flag = false;
        permiss = false;
        if (obj.locallyPermissibility(call))
        {
            flag = true;
            permiss = true;
            for (int i = obj.stable_state.index; i < executionList.size(); ++i)
            {
                if (i >= executionList.size())
                {
                    std::cerr << "Index out of bounds: localHandler" << std::endl;
                    return; // Handle out-of-bounds access
                }
                flag = checkOrder(call.type, executionList[i].type, obj.topologicalOrder);
                if (!flag)
                    break;
            }
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
                    call.call_vector_clock[i] = vector_clock[i];
                }
                executionList.push_back(call);
                obj.current_state.index++;
                stableindex = obj.stable_state.index;
                obj.updateCurrentState(call);
                //std::cout << "Exe - by local - type: " << call.type << " and value1: " << call.value1 << " Vector-Clocks: " << vector_clock[0] << "-" << vector_clock[1] << "-" << vector_clock[2] << " -call id -" << call.call_id << " -current index " << obj.current_state.index << std::endl;
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
        return -1; // Return -1 if no match is found
    }

    bool queueHandler(bool addorRemove, Call remoteCall)
    {
        // std::cout << "checkk7777" << std::endl;
        bool flag = false;
        int call_position, executed_call_position;
        // if (addorRemove)
        // std::lock_guard<std::mutex> lock(mtx);

        for (int i = 0; i < queuedList.size(); i++)
        {
            if (i >= queuedList.size())
            {
                std::cerr << "Index out of bounds: queueHandler" << std::endl;
                return false; // Handle out-of-bounds access
            }
            if (compareVectorClocks(remoteCall.call_vector_clock, queuedList[i].call_vector_clock) == 2)
            {
                if (addorRemove)
                {
                    queuedList.push_back(remoteCall);
                    //std::cout << "Exe - queued - type: " << remoteCall.type << " and value1: " << remoteCall.value1 << std::endl;
                }
                return true;
            }
        }

        call_position = findPosition(remoteCall.type, obj.topologicalOrder);
        for (int i = obj.stable_state.index; i < executionList.size(); i++)
        {
            if (i >= executionList.size())
            {
                std::cerr << "Index out of bounds: queueHandler" << std::endl;
                return false; // Handle out-of-bounds access
            }
            executed_call_position = findPosition(executionList[i].type, obj.topologicalOrder);

            if (call_position < executed_call_position)
            {
                if (compareVectorClocks(remoteCall.call_vector_clock, executionList[i].call_vector_clock) == 2)
                {
                    if (addorRemove)
                    {
                        queuedList.push_back(remoteCall);
                        //std::cout << "Exe - queued - type: " << remoteCall.type << " and value1: " << remoteCall.value1 << std::endl;
                    }
                    return true;
                }
            }
        }

        return false;
    }

    bool remoteHandler(bool addorRemove, Call call)
    {
        /*if (addorRemove) {
            for (int i = 0; i < number_of_nodes; i++) {
                if (i >= number_of_nodes || i >= call.call_vector_clock.size()) {
                    std::cerr << "Index out of bounds: remoteHandler" << std::endl;
                    return false; // Handle out-of-bounds access
                }
                remote_verctor_clocks[call.node_id - 1][i] = call.call_vector_clock[i];
            }
        }*/
        // std::cout << "checkk4444" << std::endl;
        bool add_queue_flag;

        add_queue_flag = queueHandler(addorRemove, call);

        // if (addorRemove)
        // std::lock_guard<std::mutex> lock(mtx);
        if (!add_queue_flag)
        {
            // std::lock_guard<std::mutex> lock(mtx_ack);
            int it = findPosition(call.type, obj.topologicalOrder);

            if (it != -1)
            {
                auto pos = std::next(executionList.begin(), (obj.stable_state.index));
                auto end = executionList.end();
                while (pos != end)
                {
                    int jt = findPosition(pos->type, obj.topologicalOrder);
                    if (jt > it)
                    {
                        break;
                    }
                    ++pos;
                }
                auto index = std::distance(executionList.begin(), pos);
                executionList.insert(pos, call);
                obj.send_ack = true;
                obj.send_ack_call.type = "Ack";
                obj.send_ack_call.node_id = call.node_id;
                obj.send_ack_call.call_id = call.call_id;
                obj.send_ack_call.value1 = call.value1;

                // Create a new list and update the shared pointer
                auto new_ack_list = std::make_shared<std::vector<Call>>(*obj.send_ack_call_list);
                new_ack_list->push_back(obj.send_ack_call);
                std::atomic_store(&obj.send_ack_call_list, new_ack_list);
                obj.ackindex.store(obj.ackindex.load() + 1);
                obj.current_state.index++;
                obj.updateCurrentState(call);
                vector_clock[call.node_id - 1]++;
                //std::cout << "Exe - by remote - type: " << call.type << " and value1: " << call.value1 << " Vector-Clocks: " << vector_clock[0] << "-" << vector_clock[1] << "-" << vector_clock[2] << " -call id -" << call.call_id << " -current index " << obj.current_state.index << " pos " << index << " size " << executionList.size() << std::endl;
                // std::cout << "checkk55555" << std::endl;
                return true;
            }
            // std::cout << "checkk6666" << std::endl;
        }
        return false;
    }

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
            // std::cout << "Exe - stablized - type: " << executionList[i].type << " and value1: " << executionList[i].value1 << " -stable index " << obj.stable_state.index << std::endl;
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
    }
    void updateAcksTable(Call call)
    {
        obj.acks[call.node_id - 1][call.call_id]++;
        // std::cout << "Exe - by update ack - type: " << call.type << " and value1: " << " -call id -" << call.call_id << "acks -- " << obj.acks[call.node_id - 1][call.call_id] << std::endl;
    }
    void stabilizerWithAck()
    {
        // std::lock_guard<std::mutex> lock(mtx);
        // std::cout << "checkk88888" << std::endl;
        int i = obj.stable_state.index;
        if (i >= executionList.size())
        {
            // std::cerr << "Index out of bounds: stabilizerWithAck" << std::endl;
            return;
        }

        bool stable = true;
        while (stable && i < executionList.size())
        {
            stable = false;
            // std::cout << "checkk88888.1111" << std::endl;
            if (executionList[i].node_id == node_id)
            {
                if (obj.acks[executionList[i].node_id - 1][executionList[i].call_id] == (number_of_nodes - 1))
                {
                    stable = true;
                }
            }

            else
            {
                // std::cout << "checkk88888.2222" << std::endl;
                if (obj.acks[executionList[i].node_id - 1][executionList[i].call_id] == (number_of_nodes - 2))
                {
                    stable = true;
                }
            }
            // std::cout << "checkk88888.3333" << std::endl;
            if (stable)
            {
                // std::cout << "checkk88888.444444" << std::endl;

                //std::cout << "Exe - stablized - type: " << executionList[i].type << " and value1: " << executionList[i].value1 << " -stable index " << obj.stable_state.index << std::endl;
                obj.updateStableState(executionList[i]);
                // std::cout << "checkk99999" << std::endl;
                i++;
                obj.stable_state.index++;
                obj.waittobestable.store(obj.waittobestable.load() + 1);
                if (!queuedList.empty())
                {
                    for (auto it = queuedList.begin(); it != queuedList.end();)
                    {

                        bool remove_flag;
                        remove_flag = remoteHandler(false, *it);
                        if (remove_flag)
                            it = queuedList.erase(it);
                        else
                            ++it;
                    }
                }
                // std::cout << "lock is free -with stablizer " << std::endl;
            }
        }
        // std::cout << "checkk8888.5555" << std::endl;
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
        // stabilizer(sharedexecutionList);
    }
};
