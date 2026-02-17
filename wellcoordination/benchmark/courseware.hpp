#pragma once
#include "replicated-object.hpp"

class CoursewareState
{
public:
  int index;
  std::unordered_set<int> student_ids;
  std::unordered_set<int> course_ids;
  std::unordered_set<std::pair<int, int>, pair_hash> enroll;

  // Default constructor
  CoursewareState() : index(0), student_ids(), course_ids() {}
};

class Courseware
{
public:
  CoursewareState current_state;
  CoursewareState stable_state;
  bool send_ack;
  std::shared_ptr<std::vector<Call>> send_ack_call_list = std::make_shared<std::vector<Call>>();
  std::atomic<int> waittobestable;
  std::atomic<int> ackindex;
  Call send_ack_call;
  int num_nodes;

  Courseware()
  {
    send_ack = false;

    // Allocate memory for the array on the heap
    waittobestable.store(0);
    ackindex.store(0);
  }
  enum CoursewareEnum
  {
    AddCourse,
    DeleteCourse,
    Enroll,
    AddStudent,
    DeleteStudent,
    Read
  };

  std::string coursewareToString(int courseware)
  {
    switch (static_cast<CoursewareEnum>(courseware))
    {
    case AddCourse:
      return "AddCourse";
    case DeleteCourse:
      return "DeleteCourse";
    case Enroll:
      return "Enroll";
    case AddStudent:
      return "AddStudent";
    case DeleteStudent:
      return "DeleteStudent";
    case Read:
      return "Read";
    default:
      return "Invalid";
    }
  }
  bool locallyPermissibility(Call call) // For the courseware, all benchmark files should be generated on a single machine and then passed to several node replicas.
  {
    return true; // for testing performance of the system, we can assume all calls are locally permissible. You can make the following code uncommented if you want to test the performance of the system with more realistic benchmark files where not all calls are locally permissible.
    // if (call.type == "Enroll")
    // {
    //   if (current_state.student_ids.find(call.value1) != current_state.student_ids.end() && current_state.course_ids.find(call.value2) != current_state.course_ids.end())
    //     return true;
    //   else
    //     return false;
    // }
    // else
    // {
    //   return true;
    // }
    // addproject and addemployee are always permissible
    // for works on need to check employeeid and projectid
    // for deleteproject do nothing
    // for deleteemployee do nothing
    // return true;
  }

  bool checkValidcall(Call call)
  {
    if (call.type == "AddCourse" || call.type == "DeleteCourse" || call.type == "Enroll" || call.type == "AddStudent" || call.type == "DeleteStudent")
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
    if ((input1.type == "AddCourse" && input2.type == "DeleteCourse") ||
        (input1.type == "AddStudent" && input2.type == "DeleteStudent") ||
        (input1.type == "Enroll" && (input2.type == "DeleteCourse" || input2.type == "DeleteStudent")))
    {
      return true;
    }
    return false;
  }

  bool fastisreachableremote(Call &input1, const Call &input2)
  {
    if ((input1.type == "AddCourse" && input2.type == "DeleteCourse") ||
        (input1.type == "AddStudent" && input2.type == "DeleteStudent") ||
        (input1.type == "Enroll" && (input2.type == "DeleteCourse" || input2.type == "DeleteStudent")))
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
      call.type = coursewareToString(tmp_op);
      // std::cout << "readBenchFile--" << "call.type :" << call.type << std::endl;
      if (call.type != "Read")
      {

        call_id_counter++;
        call.call_id = call_id_counter;
        if (call.type == "Enroll")
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
    if (call.type == "AddCourse")
    {
      current_state.course_ids.insert(call.value1);
    }
    else if (call.type == "DeleteCourse")
    {
      current_state.course_ids.erase(call.value1);
    }
    else if (call.type == "Endroll")
    {
      current_state.enroll.insert(std::make_pair(call.value1, call.value2));
    }
    else if (call.type == "AddStudent")
    {
      current_state.student_ids.insert(call.value1);
    }
    else if (call.type == "DeleteStudent")
    {
      current_state.student_ids.erase(call.value1);
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
    if (call.type == "AddCourse")
    {
      stable_state.course_ids.insert(call.value1);
    }
    else if (call.type == "DeleteCourse")
    {
      stable_state.course_ids.erase(call.value1);
    }
    else if (call.type == "Enroll")
    {
      stable_state.enroll.insert(std::make_pair(call.value1, call.value2));
    }
    else if (call.type == "AddStudent")
    {
      stable_state.student_ids.insert(call.value1);
    }
    else if (call.type == "DeleteStudent")
    {
      stable_state.student_ids.erase(call.value1);
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