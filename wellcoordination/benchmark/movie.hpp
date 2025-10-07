#pragma once
#include "replicated-object.hpp"

class MovieState
{
public:
  int index;
  std::unordered_set<int> movie_ids;
  std::unordered_set<int> customer_ids;


  // Default constructor
  MovieState() : index(0), movie_ids(), customer_ids() {}
};

class Movie
{
public:
  MovieState current_state;
  MovieState stable_state;
  std::vector<Call> topologicalOrder;
  bool send_ack;
  std::shared_ptr<std::vector<Call>> send_ack_call_list = std::make_shared<std::vector<Call>>();
  std::atomic<int> waittobestable;
  std::atomic<int> ackindex;
  Call send_ack_call;
  int num_nodes;

  Movie()
  {
    send_ack = false;

    // Allocate memory for the array on the heap
    waittobestable.store(0);
    ackindex.store(0);
  }
  enum MovieEnum
  {
    AddMovie,
    DeleteMovie,
    AddCustomer,
    DeleteCustomer,
    Read,
  };

  std::string movieToString(int movie)
  {
    switch (static_cast<MovieEnum>(movie))
    {
    case AddMovie:
      return "AddMovie";
    case DeleteMovie:
      return "DeleteMovie";
    case AddCustomer:
      return "AddCustomer";
    case DeleteCustomer:
      return "DeleteCustomer";
    case Read:
      return "Read";
    default:
      return "Invalid";
    }
  }
  bool locallyPermissibility(Call call) // For the movie, all benchmark files should be generated on a single machine and then passed to several node replicas.
  {
    return true;
  }
  // addproject and addemployee are always permissible
  // for works on need to check employeeid and projectid
  // for deleteproject do nothing
  // for deleteemployee do nothing
  // return true;

  bool
  checkValidcall(Call call)
  {
    if (call.type == "AddMovie" || call.type == "DeleteMovie" || call.type == "AddCustomer" || call.type == "DeleteCustomer")
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
    if ((input1.type == "AddMovie" && input2.type == "DeleteMovie") ||
        (input1.type == "AddCustomer" && input2.type == "DeleteCustomer"))
    {
      return true;
    }
    return false;
  }

  bool fastisreachableremote(Call &input1, const Call &input2)
  {
    if ((input1.type == "AddMovie" && input2.type == "DeleteMovie") ||
        (input1.type == "AddCustomer" && input2.type == "DeleteCustomer"))
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
      // call.type = movieToString(std::stoi(line.substr(0, spacePos)));
      //::cout << "readBenchFile--" << "expected_calls :" << call.type << std::endl;

      std::istringstream iss(line);
      int tmp_op;
      std::string temp;

      iss >> tmp_op;
      call.type = movieToString(tmp_op);
      // std::cout << "readBenchFile--" << "call.type :" << call.type << std::endl;
      if (call.type != "Read")
      {

        call_id_counter++;
        call.call_id = call_id_counter;

        iss >> call.value1;
        call.value2 = 0;
        // std::cout << "readBenchFile--" << "call.value1 :" << call.value1 << std::endl;
      }
      call.node_id = id;
      // node_id = id;
      call.stable = false;
      calls.push_back(call);
    }
  }
  void updateCurrentState(Call call)
  {
    if (call.type == "AddMovie")
    {
      current_state.movie_ids.insert(call.value1);
    }
    else if (call.type == "DeleteMovie")
    {
      current_state.movie_ids.erase(call.value1);
    }
    else if (call.type == "AddCustomer")
    {
      current_state.customer_ids.insert(call.value1);
    }
    else if (call.type == "DeleteCustomer")
    {
      current_state.customer_ids.erase(call.value1);
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
    if (call.type == "AddMovie")
    {
      stable_state.movie_ids.insert(call.value1);
    }
    else if (call.type == "DeleteMovie")
    {
      stable_state.movie_ids.erase(call.value1);
    }
    else if (call.type == "AddCustomer")
    {
      stable_state.customer_ids.insert(call.value1);
    }
    else if (call.type == "DeleteCustomer")
    {
      stable_state.customer_ids.erase(call.value1);
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