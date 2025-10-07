#pragma once
#include "replicated-object.hpp"

class CounterState
{
public:
  int index;
  int localcounter;
  int remotecounter;
  
  // Default constructor
  CounterState() : index(0), localcounter(0), remotecounter(0) {}
};

class Counter
{
public:
  CounterState current_state;
  std::atomic<int> waittobestable;

  Counter()
  {
    waittobestable.store(0);
  }
  enum CounterEnum
  {
    Update,
    Read
  };

  std::string counterToString(int counter)
  {
    switch (static_cast<CounterEnum>(counter))
    {
    case Update:
      return "Update";
    case Read:
      return "Read";
    default:
      return "Invalid";
    }
  }

  bool checkValidcall(Call call)
  {
    if (call.type == "Update")
      true;
    else
      false;
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
      call.type = counterToString(tmp_op);
      // std::cout << "readBenchFile--" << "call.type :" << call.type << std::endl;
      if (call.type != "Read")
      {

        call_id_counter++;
        call.call_id = call_id_counter;
        if (call.type == "Update")
        {
          // std::getline(iss, temp, '-');
          // call.value1 = std::stoi(temp);
          // iss >> call.value2;
          //  std::cout << "readBenchFile--" << "call.value1 :" << call.value1 << std::endl;
          //  std::cout << "readBenchFile--" << "call.value2 :" << call.value2 << std::endl;

          iss >> call.value1;
          call.value2 = 0;
          // std::cout << "readBenchFile--" << "call.value1 :" << call.value1 << std::endl;
        }
      }
      call.node_id = id;
      // node_id = id;
      call.stable = false;
      calls.push_back(call);
    }
  }
  void updateLocal(Call &call)
  {
    if (call.type == "Update")
    {
      current_state.localcounter += call.value1;
      std::cout << "Local Counter: " << current_state.localcounter << std::endl;
      waittobestable.store(waittobestable.load() + 1);
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
  void updateRemote(Call call)
  {
    if (call.type == "Update")
    {
      current_state.remotecounter += call.value1;
      std::cout << "Remote Counter: " << current_state.remotecounter << std::endl;
      waittobestable.store(waittobestable.load() + 1);
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