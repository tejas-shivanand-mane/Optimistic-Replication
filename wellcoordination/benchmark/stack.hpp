#pragma once
#include "replicated-object.hpp"

class StackState
{
public:
  int index;
  std::vector<int> elements;

  // Default constructor
  StackState() : index(0) {}
};

class Stack
{
public:
  StackState current_state;
  StackState stable_state;
  std::vector<Call> topologicalOrder;
  DirectedGraph stackgrapgh;
  bool send_ack;
  std::shared_ptr<std::vector<Call>> send_ack_call_list = std::make_shared<std::vector<Call>>();
  std::atomic<int> waittobestable;
  std::atomic<int> ackindex;
  Call send_ack_call;
  int num_nodes;

  Stack()
  {
    // topologicalOrder.push_back(Call("AddProject", 0, 0));
    // topologicalOrder.push_back(Call("DeleteProject", 0, 0));
    // topologicalOrder.push_back(Call("WorksOn", 0, 0));
    // topologicalOrder.push_back(Call("AddEmployee", 0, 0));
    send_ack = false;

    // Allocate memory for the array on the heap
    waittobestable.store(0);
    ackindex.store(0);
  }
  enum StackEnum
  {
    Push,
    Pop,
    Read
  };

  std::string stackToString(int stack)
  {
    switch (static_cast<StackEnum>(stack))
    {
    case Push:
      return "Push";
    case Pop:
      return "Pop";
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

  bool checkValidcall(Call call)
  {
    if (call.type == "Push" || call.type == "Pop")
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
    if (((input1.type == "Push") && (input2.type == "Push") && (input1.node_id < input2.node_id)) ||
        ((input1.type == "Push") && (input2.type == "Pop")))
    {
      return true;
    }
    return false;
  }
  bool fastisreachableremote(Call &input1, const Call &input2)
  {
    if (
        ((input1.type == "Push") && (input2.type == "Pop"))&& 1==compareVectorClocks(input1.call_vector_clock,input2.call_vector_clock)) //check it is concurrent?
    {
      return true;
    }
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
      call.type = stackToString(tmp_op);
      // std::cout << "readBenchFile--" << "call.type :" << call.type << std::endl;
      if (call.type != "Read")
      {

        call_id_counter++;
        call.call_id = call_id_counter;
        if (call.type == "Push")
        {
          //std::getline(iss, temp, '-');
          //call.value1 = std::stoi(temp);
          //iss >> call.value2;
          // std::cout << "readBenchFile--" << "call.value1 :" << call.value1 << std::endl;
          // std::cout << "readBenchFile--" << "call.value2 :" << call.value2 << std::endl;

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

  void updateCurrentState(Call call)
  {
    if (call.type == "Push")
    {
      current_state.elements.push_back(call.value1);
    }
    else if (call.type == "Pop")
    {
      if (!current_state.elements.empty())
      {
        current_state.elements.pop_back();
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
    if (call.type == "Push")
    {
      stable_state.elements.push_back(call.value1);
    }
    else if (call.type == "Pop")
    {
      if (!stable_state.elements.empty())
      {
        stable_state.elements.pop_back();
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
};