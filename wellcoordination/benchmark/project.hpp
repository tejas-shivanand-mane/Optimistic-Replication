#pragma once
#include "replicated-object.hpp"

class ProjectState
{
public:
  int index;
  std::unordered_set<int> employee_ids;
  std::unordered_set<int> project_ids;
  std::unordered_set<std::pair<int, int>, pair_hash> works;

  // Default constructor
  ProjectState() : index(0), employee_ids(), project_ids() {}
};

class Project
{
public:
  ProjectState current_state;
  ProjectState stable_state;
  std::vector<Call> topologicalOrder;
  DirectedGraph projectgrapgh;
  bool send_ack;
  std::shared_ptr<std::vector<Call>> send_ack_call_list = std::make_shared<std::vector<Call>>();
  std::atomic<int> waittobestable;
  std::atomic<int> ackindex;
  Call send_ack_call;
  int num_nodes;

  Project()
  {
    // topologicalOrder.push_back(Call("AddProject", 0, 0));
    // topologicalOrder.push_back(Call("DeleteProject", 0, 0));
    // topologicalOrder.push_back(Call("WorksOn", 0, 0));
    // topologicalOrder.push_back(Call("AddEmployee", 0, 0));
    send_ack = false;

    projectgrapgh.addEdge("AddEmployee", "WorksOn");
    projectgrapgh.addEdge("AddProject", "WorksOn");
    projectgrapgh.addEdge("WorksOn", "DeleteProject");

    // Allocate memory for the array on the heap
    waittobestable.store(0);
    ackindex.store(0);
  }
  enum ProjectEnum
  {
    AddProject,
    DeleteProject,
    WorksOn,
    AddEmployee,
    DeleteEmployee,
    Read
  };

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
    case DeleteEmployee:
      return "DeleteEmployee";
    case Read:
      return "Read";
    default:
      return "Invalid";
    }
  }
  bool locallyPermissibility(Call call) // For the project, all benchmark files should be generated on a single machine and then passed to several node replicas.
  {
    // return true; //if you can not generate the benchmark files on a single machine, then you can make this line uncommented.
    if (call.type == "WorksOn")
    {
      if (current_state.employee_ids.find(call.value1) != current_state.employee_ids.end() && current_state.project_ids.find(call.value2) != current_state.project_ids.end())
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
    // for deleteemployee do nothing
    // return true;
  }

  bool checkValidcall(Call call)
  {
    if (call.type == "AddProject" || call.type == "DeleteProject" || call.type == "WorksOn" || call.type == "AddEmployee" || call.type == "DeleteEmployee")
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
    if ((input1.type == "AddEmployee" && input2.type == "DeleteEmployee") ||
        (input1.type == "AddProject" && input2.type == "DeleteProject") ||
        (input1.type == "WorksOn" && (input2.type == "DeleteEmployee" || input2.type == "DeleteProject")))
    {
      return true;
    }
    return false;
  }

  bool fastisreachableremote(Call &input1, const Call &input2)
  {
    if ((input1.type == "AddEmployee" && input2.type == "DeleteEmployee") ||
        (input1.type == "AddProject" && input2.type == "DeleteProject") ||
        (input1.type == "WorksOn" && (input2.type == "DeleteEmployee" || input2.type == "DeleteProject")))
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
      call.type = projectToString(tmp_op);
      // std::cout << "readBenchFile--" << "call.type :" << call.type << std::endl;
      if (call.type != "Read")
      {

        call_id_counter++;
        call.call_id = call_id_counter;
        if (call.type == "WorksOn")
        {
          std::getline(iss, temp, '-');
          call.value1 = std::stoi(temp);
          iss >> call.value2;
          // std::cout << "readBenchFile--" << "call.value1 :" << call.value1 << std::endl;
          // std::cout << "readBenchFile--" << "call.value2 :" << call.value2 << std::endl;
        }
        else
        {
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
    if (call.type == "AddProject")
    {
      current_state.project_ids.insert(call.value1);
    }
    else if (call.type == "DeleteProject")
    {
      current_state.project_ids.erase(call.value1);
    }
    else if (call.type == "WorksOn")
    {
      current_state.works.insert(std::make_pair(call.value1, call.value2));
    }
    else if (call.type == "AddEmployee")
    {
      current_state.employee_ids.insert(call.value1);
    }
    else if (call.type == "DeleteEmployee")
    {
      current_state.employee_ids.erase(call.value1);
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
    if (call.type == "AddProject")
    {
      stable_state.project_ids.insert(call.value1);
    }
    else if (call.type == "DeleteProject")
    {
      stable_state.project_ids.erase(call.value1);
    }
    else if (call.type == "WorksOn")
    {
      stable_state.works.insert(std::make_pair(call.value1, call.value2));
    }
    else if (call.type == "AddEmployee")
    {
      stable_state.employee_ids.insert(call.value1);
    }
    else if (call.type == "DeleteEmployee")
    {
      stable_state.employee_ids.erase(call.value1);
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