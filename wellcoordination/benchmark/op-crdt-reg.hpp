#pragma once
#include "replicated-object.hpp"

class OpRegState
{
public:
  int index;

  int value;
  int timestamp;
  //std::set<int> elements;

  // Default constructor
  OpRegState() : index(0), value(0), timestamp(0) {}
};

class Opcrdtreg
{
public:
  OpRegState current_state;
  OpRegState stable_state;
  std::vector<Call> topologicalOrder;
  //DirectedGraph setgrapgh;
  bool send_ack;
  std::shared_ptr<std::vector<Call>> send_ack_call_list = std::make_shared<std::vector<Call>>();
  std::atomic<int> waittobestable;
  std::atomic<int> ackindex;
  Call send_ack_call;
  std::vector<std::vector<int>> acks;

  Opcrdtreg()
  {
    // topologicalOrder.push_back(Call("AddProject", 0, 0));
    // topologicalOrder.push_back(Call("DeleteProject", 0, 0));
    // topologicalOrder.push_back(Call("WorksOn", 0, 0));
    // topologicalOrder.push_back(Call("AddEmployee", 0, 0));
    send_ack = false;

    // Allocate memory for the array on the heap
    acks.resize(8, std::vector<int>(350000, 0));
    waittobestable.store(0);
    ackindex.store(0);
  }
  enum RegEnum
  {
    Update,
    Read
  };

  std::string regToString(int reg)
  {
    switch (static_cast<RegEnum>(reg))
    {
    case Update:
      return "Update";
    case Read:
      return "Read";
    default:
      return "Invalid";
    }
  }
  bool locallyPermissibility(Call call)
  {
    return true;
    // addproject and addemployee are always permissible
    // for works on need to check employeeid and projectid
    // for deleteproject do nothing
    // return true;
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

    // Debug print to verify serialized buffer
    // std::cout << "Serialized buffer: ";
    // for (size_t i = 0; i < len; ++i)
    //{
    // std::cout << std::hex << static_cast<int>(buffer[i]) << " ";
    //}
    // std::cout << std::endl;

    return len + sizeof(uint64_t) + 2 * sizeof(uint64_t);
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
    std::vector<int> call_vector_clock(8, 0);
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
  }

  bool checkValidcall(Call call)
  {
    if (call.type == "Update")
      true;
    else
      false;
  }
  int compareVectorClocks(std::vector<int> vc1, std::vector<int> vc2)
  {
    // return 0 if vc1 is causally before vc2
    // return 1 if vc1 is concurrent with vc2
    // return 2 if vc1 is causally after vc2
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
  bool fastisreachablelocal(Call &input1, const Call &input2)
  {

    return false;
  }
  bool fastisreachableremote(Call &input1, const Call &input2)
  {

    return false;
  }

 void readBenchFile(const std::string &filename, int &expected_calls, int id, int numnodes, Call call, std::vector<Call> &calls)
  {
    std::ifstream infile(filename);
    // number_of_nodes = numnodes;
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
      // std::cout << "readBenchFile--" << "expected_calls :" << expected_calls << std::endl;
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
      call.type = regToString(tmp_op);
      // std::cout << "readBenchFile--" << "call.type :" << call.type << std::endl;
      if (call.type != "Read")
      {

        call_id_counter++;
        call.call_id = call_id_counter;
        if (call.type == "Update")
        {
          //std::getline(iss, temp, '-');
          //call.value1 = std::stoi(temp);
          //iss >> call.value2;
          // std::cout << "readBenchFile--" << "call.value1 :" << call.value1 << std::endl;
          // std::cout << "readBenchFile--" << "call.value2 :" << call.value2 << std::endl;

          iss >> call.value1;
          current_state.timestamp++;
          call.value2 = current_state.timestamp;
          // std::cout << "readBenchFile--" << "call.value1 :" << call.value1 << std::endl;
        }
      }
      call.node_id = id;
      // node_id = id;
      call.stable = false;
      calls.push_back(call);
    }
  }
  void updateCurrentState(Call call)
  {
    if (call.type == "Update")
    {
      if (current_state.timestamp < call.value2)
      {
        current_state.timestamp = call.value2;
        current_state.value = call.value1;
      }
    }
    else if (call.type == "Read")
    {
      // Do nothing
    }
    else
    {
      // std::cout << "wrong method name" << std::endl;
      ;
    }
  }
  void updateStableState(Call call)
  {
    if (call.type == "Update")
    {
      stable_state.timestamp=current_state.timestamp;
      stable_state.value=current_state.value;
    }
    else if (call.type == "Read")
    {
      // Do nothing
    }
    else
    {
      // std::cout << "wrong method name" << std::endl;
      ;
    }
  }
};