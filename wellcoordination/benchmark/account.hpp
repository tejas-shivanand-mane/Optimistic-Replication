#pragma once
#include "replicated-object.hpp"

class AccountState
{
public:
  int index;
  int balance;


  // Default constructor
  AccountState() : index(0), balance(0) {}
};

class Account
{
public:
  AccountState current_state;
  AccountState stable_state;
  std::vector<Call> topologicalOrder;

  Account()
  {
    topologicalOrder.push_back(Call("WithDraw", 0, 0));
    topologicalOrder.push_back(Call("Deposit", 0, 0));
  }
  enum AccountEnum
  {
    WithDraw,
    Deposit,
    Read
  };

  std::string projectToString(int account)
  {
    switch (static_cast<AccountEnum>(account))
    {
    case WithDraw:
      return "WithDraw";
    case Deposit:
      return "Deposit";
    case Read:
      return "Read";
    default:
      return "Invalid";
    }
  }
  bool locallyPermissibility(Call call)
  {
    if (call.type == "WithDraw")
    {
      if ((stable_state.balance- call.value1) >0 )
        return true;
      else
        return false;
    }
    else
    {
      return true;
    }
    // addproject and addemployee are always permissible
    // for works on need to check employeeid and projectid
    // for deleteproject do nothing
    // return true;
  }
  static size_t serializeCalls(Call call, uint8_t *buffer)
  {
    std::string type = call.type;
    int value1 = call.value1;
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

  Call deserializeCalls(uint8_t *buffer, std::atomic<int> *initcounter)
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

    //std::cout << "deserial--" << " n-id :" << node_id << " c-id :" << call_id << " type :" << type << " values :" << value1 << std::endl;

    return Call(type, value1, node_id, call_id, stable);
  }

  void readBenchFile(const std::string &filename, int &expected_calls, int id, int numnodes, Call call, std::vector<Call> &calls)
  {
    std::ifstream infile(filename);
    //number_of_nodes = numnodes;
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
      call.type = projectToString(tmp_op);
      // std::cout << "readBenchFile--" << "call.type :" << call.type << std::endl;
      if (call.type != "Read")
      {

        call_id_counter++;
        call.call_id = call_id_counter;

        iss >> call.value1;

      }
      call.node_id = id;
      //node_id = id;
      call.stable = false;
      calls.push_back(call);
    }
  }
  void updateCurrentState(Call call)
  {
    if (call.type == "WithDraw")
    {
      current_state.balance -= call.value1 ;
    }
    else if (call.type == "Deposit")
    {
      current_state.balance += call.value1 ;
    }
    else if (call.type == "Read")
    {
      // Do nothing
    }
    else
    {
      std::cout << "wrong method name" << std::endl;
    }
  }
  void updateStableState(Call call)
  {
    if (call.type == "WithDraw")
    {
      stable_state.balance -= call.value1 ;
    }
    else if (call.type == "Deposit")
    {
      stable_state.balance += call.value1 ;
    }
    else if (call.type == "Read")
    {
      // Do nothing
    }
    else
    {
      std::cout << "wrong method name" << std::endl;
    }
  }
};