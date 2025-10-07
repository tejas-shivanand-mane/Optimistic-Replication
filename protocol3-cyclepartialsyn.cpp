#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <unordered_set>

#define PROJECTUSECASE

#ifdef PROJECTUSECASE
#include "wellcoordination/benchmark/project.hpp"
#endif

enum ProjectEnum
{
    AddProject,
    DeleteProject,
    WorksOn,
    AddEmployee,
    Read
};

class Call
{
public:
    std::string type;
    int value1;
    int value2;
    int node_id;
    int call_id;
    bool stable;
    std::vector<int> call_vector_clock; // Add this line

    // Default constructor
    Call() : type(""), value1(0), value2(0), node_id(0), call_id(0), stable(false), call_vector_clock(8, 0) {}

    // Parameterized constructor
    Call(std::string t, int v1, int v2, int n_id = 0, int c_id = 0, bool s = false)
        : type(t), value1(v1), value2(v2), node_id(n_id), call_id(c_id), stable(s), call_vector_clock(8, 0) {}
};

// Define the Handler class
class Handler
{
public:
    std::unordered_set<std::int> employee_ids;
    std::unordered_set<std::int> project_ids;
    std::vector<int> vector_clock;

    // Default constructor
    Handler() : vector_clock(8, 0) {}

public:
    bool needAddQueue(Call call, std::vector<Call> &QueuedList)
    {

        for (size_t i = 0; i < 8; ++i)
        {
            if (call.call_vector_clock[i] > vector_clock[i])
            {
                executionList.push_back(call);
                return true;
            }
        }
    }
    void localHandler(Call call, std::vector<Call> &executionList, std::vector<Call> topologicalOrder)
    {
        std::cout << "Local Handler received a call with type: " << call.type << " and value1: " << call.value1 << std::endl;
        if (locallyPermissibility(call, call))
        {
            bool flag = true;
            for (int i = 0; i < executionList.size(); ++i)
            {
                flag = checkOrder(call.type, executionList[i].type, topologicalOrder);
                if (flag == false)
                    break;
            }
            if (flag)
            {
                executionList.push_back(call);
            }
            std::cout << call.type;
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

    void remoteHandler(Call call, std::vector<Call> &executionList, std::vector<Call> topologicalOrder)
    {
        vector_clock[call.node_id - 1]++; // update vector clock

        // Find the position of the new call in the topological order
        int it = findPosition(call.type, topologicalOrder);

        if (it != -1)
        {
            // If the call type is found in the topological order, find the correct position in the execution list
            auto pos = executionList.begin();
            while (pos != executionList.end())
            {
                int jt = findPosition(pos->type, topologicalOrder);
                if (jt > it)
                {
                    break;
                }
                ++pos;
            }
            // Insert the new call at the correct position
            executionList.insert(pos, call);
        }
        else
        {
            // If the call type is not in the topological order, just add it to the end of the execution list
            executionList.push_back(call);
        }
        // Q the call. ()
    }

    void stabilizer(Call &call, std::vector<int> localVectorClocks, std::vector<std::vector<int>> remoteVectorClocks, int nodeNum)
    {
        std::cout << "Stabilizer received a call with type: " << call.type << " and value1: " << call.value1 << std::endl;
        bool flag = true;
        while (flag)
        {
            for (int i = 0; i < (nodeNum - 1); i++)
            {
                for (int j = 0; localVectorClocks.size(); j++)
                {
                    if (remoteVectorClocks[i][j] < localVectorClocks[j])
                        flag = false;
                }
            }
        }
        if (flag)
            call.stable = true;
        // executionList.push_back(call);
    }

    int findCallIndex(const std::string &type, const std::vector<Call> &calls)
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

    bool checkOrder(const std::string &type1, const std::string &type2, const std::vector<Call> &calls)
    {
        int order1, order2;
        order1 = findCallIndex(type1, calls);
        order2 = findCallIndex(type2, calls);
        if (order2 <= order1)
            return true;
        else
            return false;
    }
    bool locallyPermissibility(Call call, Call initialState)
    {
        if (call.type == "WorksOn")
            if (employee_ids.contains(call.value1) && project_ids.contains(call.value2))
                true;
            else
                false;
        else
            return true;
        // addproject and addemployee are always permissible
        // for works on need to check employeeid and projectid
        // for deleteproject do nothing
        // return true;
    }

    std::string projectToString(int project)
    {
        switch (static_cast<ProjectEnum>(project))
        {
        case AddProject:
            return "AddProject";
        case DeleteProject:
            return "DeleteProject";
        case WorksOn:
            return "WorksOn";
        case AddEmployee:
            return "AddEmployee";
        case Read:
            return "Read";
        default:
            return "Invalid";
        }
    }

    void readBenchFile(const std::string &filename, int &expected_calls, int id, Call call, std::vector<Call> &calls)
    {
        std::ifstream infile(filename);
        if (!infile)
        {
            std::cerr << "Could not open file: " << filename << std::endl;
            return;
        }

        std::string line;
        if (std::getline(infile, line))
        {
            // Remove the '#' character and convert to integer
            expected_calls = std::stoi(line.substr(1));
            std::cout << "readBenchFile--" << "expected_calls :" << expected_calls << std::endl;
        }
        int call_id_counter = 0;
        while (std::getline(infile, line))
        {
            // Call call;
            // size_t spacePos = line.find(' ');
            // call.type = projectToString(std::stoi(line.substr(0, spacePos)));
            //::cout << "readBenchFile--" << "expected_calls :" << call.type << std::endl;

            std::istringstream iss(line);
            int tmp_op;
            std::string temp;

            iss >> tmp_op;
            call.type = projectToString(tmp_op);
            std::cout << "readBenchFile--" << "call.type :" << call.type << std::endl;
            if (call.type != "Read")
            {

                call_id_counter++;
                vector_clock[id - 1]++;
                call.call_id = call_id_counter;
                call.call_vector_clock[id - 1] = vector_clock[id - 1];
                if (call.type == "WorksOn")
                {
                    std::getline(iss, temp, '-');
                    call.value1 = std::stoi(temp);
                    iss >> call.value2;
                    std::cout << "readBenchFile--" << "call.value1 :" << call.value1 << std::endl;
                    std::cout << "readBenchFile--" << "call.value2 :" << call.value2 << std::endl;
                }
                else
                {
                    iss >> call.value1;
                    call.value2 = 0;
                    std::cout << "readBenchFile--" << "call.value1 :" << call.value1 << std::endl;
                }
            }
            call.node_id = id;
            call.stable = false;
            calls.push_back(call);
        }
    }
    static size_t serializeCalls(Call call, uint8_t *buffer)
    {
        std::string type = call.type;
        int value1 = call.value1;
        int value2 = call.value2;
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

    Call deserializeCalls(uint8_t *buffer)
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
        std::vector<int> call_vector_clock(8, 0);
        for (int &vc : call_vector_clock)
        {
            vc = *reinterpret_cast<int *>(start);
            start += sizeof(vc);
        }

        std::cout << "deserial--" << " n-id :" << node_id << " c-id :" << call_id << " type :" << type << " values :" << value1 << " - " << value2 << std::endl;
        return Call(type, value1, value2, node_id, call_id, stable);
    }

    void internalExecute(Call call)
    {
        // std::string out = execute(call);
        //  calls_applied_crdt[call.method_type][origin]++;
        // return out;
    }

    void internalDownstreamExecute(Call call, size_t origin, bool b)
    {
        // executeDownstream(call, b);
        // calls_applied[call.method_type][origin]++;
    }
};

/*int main() {
    // Create a Call object
    //Call call("Test", 123);

    // Create a vector of Call objects (executionList)
    std::vector<Call> executionList, topologicalOrder;

    topologicalOrder.push_back(Call("AddProject", 0));
    topologicalOrder.push_back(Call("DeleteProject", 0));
    topologicalOrder.push_back(Call("WorksOn", 0));
    topologicalOrder.push_back(Call("AddEmployee", 0));

    // Create a Handler object
    Handler handler;
    int index=234234;
    index= handler.findCallIndex("WorksOn", topologicalOrder);

    // Use the localHandler, remoteHandler and stabilizer functions
    //handler.localHandler(call, executionList, topologicalOrder);
    //handler.remoteHandler(call, executionList, topologicalOrder);
    //handler.stabilizer(call, executionList);

    // Print the size of executionList
    //std::cout << "Size of execution list: " << executionList.size() << std::endl;
    std::cout << index <<std::endl;

    int expected_calls;
    std::vector<Call> calls;
    //handler.readBenchFile("/path/to/your/file.txt", expected_calls, calls);

    std::cout << "Number of writes: " << expected_calls << std::endl;
    //for (const auto& call : calls) {
       // std::cout << "Method type: " << call.methodType << ", Value: " << call.value1 << std::endl;
   // }
    return 0;
}*/
