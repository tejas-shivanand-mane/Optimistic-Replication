#pragma once
#include "replicated-object.hpp"

class ComplexCoursewareState
{
public:
  struct StudentRow
  {
    int first_name;
    int last_name;
  };

  struct InstructorRow
  {
    int first_name;
    int last_name;
    int department;
  };

  struct CourseRow
  {
    int course_name;
    int semester;
    int department;
  };

  int index;

  // Base tables
  std::unordered_map<int, StudentRow> students;       // STUDENT: <student_id -> (first,last)>
  std::unordered_map<int, InstructorRow> instructors; // INSTRUCTOR: <instructor_id -> (first,last,dept)>
  std::unordered_map<int, CourseRow> courses;         // COURSE: <course_id -> (name,semester,dept)>
  std::unordered_set<int> departments;                // DEPARTMENT: <department>

  // Link tables
  std::unordered_set<std::pair<int, int>, pair_hash> enroll; // ENROLLMENT: <course_id, student_id>
  std::unordered_set<std::pair<int, int>, pair_hash> teach;  // TEACHING: <course_id, instructor_id>

  // Enforce UNIQUE course_name (course_name -> course_id)
  std::unordered_map<int, int> course_name_index;

  // Default constructor
  ComplexCoursewareState()
      : index(0),
        students(),
        instructors(),
        courses(),
        departments(),
        enroll(),
        teach(),
        course_name_index()
  {
  }
};

class ComplexCourseWare
{
public:
  ComplexCoursewareState current_state;
  ComplexCoursewareState stable_state;
  bool send_ack;
  std::shared_ptr<std::vector<Call>> send_ack_call_list = std::make_shared<std::vector<Call>>();
  std::atomic<int> waittobestable;
  std::atomic<int> ackindex;
  Call send_ack_call;
  int num_nodes;

  ComplexCourseWare()
  {
    send_ack = false;

    // Allocate memory for the array on the heap
    waittobestable.store(0);
    ackindex.store(0);
  }
  enum CoursewareEnum
  {
    AddStudentForce,
    DeleteStudentCascade,
    UpdateStudentIDCascade,
    UpdateStudentOther,

    AddInstructorForce,
    DeleteInstructorCascade,
    UpdateInstructorIDCascade,
    UpdateInstructorOther,

    AddCourseForce,
    DeleteCourseCascade,
    UpdateCourseIDCascade,
    UpdateCourseNameForce,
    UpdateCourseOther,

    AddDepartment,
    DeleteDepartmentNoAction,

    Enroll,
    WithdrawEnrollment,

    Teach,
    WithdrawTeaching,

    Read
  };

  std::string coursewareToString(int courseware)
  {
    switch (static_cast<CoursewareEnum>(courseware))
    {
    case AddStudentForce:
      return "AddStudentForce";
    case DeleteStudentCascade:
      return "DeleteStudentCascade";
    case UpdateStudentIDCascade:
      return "UpdateStudentIDCascade";
    case UpdateStudentOther:
      return "UpdateStudentOther";

    case AddInstructorForce:
      return "AddInstructorForce";
    case DeleteInstructorCascade:
      return "DeleteInstructorCascade";
    case UpdateInstructorIDCascade:
      return "UpdateInstructorIDCascade";
    case UpdateInstructorOther:
      return "UpdateInstructorOther";

    case AddCourseForce:
      return "AddCourseForce";
    case DeleteCourseCascade:
      return "DeleteCourseCascade";
    case UpdateCourseIDCascade:
      return "UpdateCourseIDCascade";
    case UpdateCourseNameForce:
      return "UpdateCourseNameForce";
    case UpdateCourseOther:
      return "UpdateCourseOther";

    case AddDepartment:
      return "AddDepartment";
    case DeleteDepartmentNoAction:
      return "DeleteDepartmentNoAction";

    case Enroll:
      return "Enroll";
    case WithdrawEnrollment:
      return "WithdrawEnrollment";

    case Teach:
      return "Teach";
    case WithdrawTeaching:
      return "WithdrawTeaching";

    case Read:
      return "Read";
    default:
      return "Invalid";
    }
  }

  bool locallyPermissibility(Call call) // For the courseware, all benchmark files should be generated on a single machine and then passed to several node replicas.
  {
    return true; // for testing performance of the system, we can assume all calls are locally permissible. You can make the following code uncommented if you want to test the performance of the system with more realistic benchmark files where not all calls are locally permissible.
    // return true; //if you can not generate the benchmark files on a single machine, then you can make this line uncommented.

    // Assumed argument encoding in Call:
    // - AddInstructor(<instructor_id, first_name, last_name, department>): value1=id, value2=first, value3=last, value4=dept
    // - AddCourseForce(<course_id, class_name, semester, department>):     value1=id, value2=name,  value3=sem,  value4=dept
    // - UpdateCourseOther(<course_id, semester, department>):             value1=id, value2=sem,   value3=dept
    // - DeleteDepartmentNoAction(<department>):                           value1=dept
    // - Enroll(<course_id, student_id>):                                  value1=course, value2=student
    // - Teach(<course_id, instructor_id>):                                value1=course, value2=instructor

    // if (call.type == "AddInstructorForce")
    // {
    //   const int dept = call.value4;
    //   return (current_state.departments.find(dept) != current_state.departments.end());
    // }
    // else if (call.type == "AddCourseForce")
    // {
    //   const int dept = call.value4;
    //   return (current_state.departments.find(dept) != current_state.departments.end());
    // }
    // else if (call.type == "UpdateCourseOther")
    // {
    //   const int course_id = call.value1;
    //   const int dept = call.value3;

    //   // must be able to reference an existing course tuple, and dept must exist
    //   if (current_state.courses.find(course_id) == current_state.courses.end())
    //     return false;

    //   return (current_state.departments.find(dept) != current_state.departments.end());
    // }
    // else if (call.type == "DeleteDepartmentNoAction")
    // {
    //   const int dept = call.value1;

    //   // If dept doesn't exist, allow as a no-op.
    //   if (current_state.departments.find(dept) == current_state.departments.end())
    //     return true;

    //   // Reject if referenced by any instructor
    //   for (const auto &kv : current_state.instructors)
    //   {
    //     if (kv.second.department == dept)
    //       return false;
    //   }

    //   // Reject if referenced by any course
    //   for (const auto &kv : current_state.courses)
    //   {
    //     if (kv.second.department == dept)
    //       return false;
    //   }

    //   return true;
    // }
    // else if (call.type == "Enroll")
    // {
    //   const int course_id = call.value1;
    //   const int student_id = call.value2;

    //   return (current_state.courses.find(course_id) != current_state.courses.end() &&
    //           current_state.students.find(student_id) != current_state.students.end());
    // }
    // else if (call.type == "Teach")
    // {
    //   const int course_id = call.value1;
    //   const int instructor_id = call.value2;

    //   // NOTE: your pasted spec line says "TEACHING table" here; assuming it meant INSTRUCTOR table.
    //   return (current_state.courses.find(course_id) != current_state.courses.end() &&
    //           current_state.instructors.find(instructor_id) != current_state.instructors.end());
    // }
    // else
    // {
    //   return true;
    // }
  }

  bool checkValidcall(Call call)
  {
    if (call.type == "AddStudentForce" ||
        call.type == "DeleteStudentCascade" ||
        call.type == "UpdateStudentIDCascade" ||
        call.type == "UpdateStudentOther" ||

        call.type == "AddInstructorForce" ||
        call.type == "DeleteInstructorCascade" ||
        call.type == "UpdateInstructorIDCascade" ||
        call.type == "UpdateInstructorOther" ||

        call.type == "AddCourseForce" ||
        call.type == "DeleteCourseCascade" ||
        call.type == "UpdateCourseIDCascade" ||
        call.type == "UpdateCourseNameForce" ||
        call.type == "UpdateCourseOther" ||

        call.type == "AddDepartment" ||
        call.type == "DeleteDepartmentNoAction" ||

        call.type == "Enroll" ||
        call.type == "WithdrawEnrollment" ||

        call.type == "Teach" ||
        call.type == "WithdrawTeaching" ||

        call.type == "Read")
      return true;
    else
      return false;
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
    if ((input1.type == "AddStudentForce" && input2.type == "AddStudentForce" && input1.node_id < input2.node_id) ||
        (input1.type == "AddStudentForce" && (input2.type == "DeleteStudentCascade" || input2.type == "UpdateStudentIDCascade")) ||
        (input1.type == "UpdateStudentIDCascade" && input2.type == "UpdateStudentIDCascade" && input1.node_id < input2.node_id) ||
        (input1.type == "UpdateStudentOther" && input2.type == "UpdateStudentOther" && input1.node_id < input2.node_id) ||
        (input1.type == "UpdateStudentOther" && (input2.type == "AddStudentForce" || input2.type == "UpdateStudentIDCascade")) ||

        (input1.type == "AddInstructorForce" && input2.type == "AddInstructorForce" && input1.node_id < input2.node_id) ||
        (input1.type == "AddInstructorForce" && (input2.type == "DeleteInstructorCascade" ||
                                                 input2.type == "UpdateInstructorIDCascade" ||
                                                 input2.type == "DeleteDepartmentNoAction")) ||
        (input1.type == "UpdateInstructorIDCascade" && input2.type == "UpdateInstructorIDCascade" && input1.node_id < input2.node_id) ||
        (input1.type == "UpdateInstructorOther" && input2.type == "UpdateInstructorOther" && input1.node_id < input2.node_id) ||
        (input1.type == "UpdateInstructorOther" && (input2.type == "AddInstructorForce" ||
                                                    input2.type == "UpdateInstructorIDCascade" ||
                                                    input2.type == "DeleteDepartmentNoAction")) ||

        (input1.type == "AddCourseForce" && input2.type == "AddCourseForce" && input1.node_id < input2.node_id) ||
        (input1.type == "AddCourseForce" && (input2.type == "DeleteCourseCascade" ||
                                             input2.type == "UpdateCourseIDCascade" ||
                                             input2.type == "DeleteDepartmentNoAction")) ||
        (input1.type == "UpdateCourseIDCascade" && input2.type == "UpdateCourseIDCascade" && input1.node_id < input2.node_id) ||
        (input1.type == "UpdateCourseNameForce" && input2.type == "UpdateCourseNameForce" && input1.node_id < input2.node_id) ||
        (input1.type == "UpdateCourseNameForce" && (input2.type == "AddCourseForce" || input2.type == "UpdateCourseIDCascade")) ||
        (input1.type == "UpdateCourseOther" && input2.type == "UpdateCourseOther" && input1.node_id < input2.node_id) ||
        (input1.type == "UpdateCourseOther" && (input2.type == "AddCourseForce" ||
                                                input2.type == "UpdateCourseIDCascade" ||
                                                input2.type == "DeleteDepartmentNoAction")) ||

        (input1.type == "AddDepartment" && input2.type == "DeleteDepartmentNoAction") ||

        (input1.type == "Enroll" && (input2.type == "DeleteStudentCascade" ||
                                     input2.type == "UpdateStudentIDCascade" ||
                                     input2.type == "DeleteCourseCascade" ||
                                     input2.type == "UpdateCourseIDCascade" ||
                                     input2.type == "WithdrawEnrollment")) ||

        (input1.type == "Teach" && (input2.type == "DeleteInstructorCascade" ||
                                    input2.type == "UpdateInstructorIDCascade" ||
                                    input2.type == "DeleteCourseCascade" ||
                                    input2.type == "UpdateCourseIDCascade" ||
                                    input2.type == "WithdrawTeaching")))
    {
      return true;
    }
    return false;
  }
  bool fastisreachableremote(Call &input1, const Call &input2)
  {
    if ((input1.type == "AddStudentForce" && input2.type == "AddStudentForce" && input1.node_id < input2.node_id) ||
        (input1.type == "AddStudentForce" && (input2.type == "DeleteStudentCascade" || input2.type == "UpdateStudentIDCascade")) ||
        (input1.type == "UpdateStudentIDCascade" && input2.type == "UpdateStudentIDCascade" && input1.node_id < input2.node_id) ||
        (input1.type == "UpdateStudentOther" && input2.type == "UpdateStudentOther" && input1.node_id < input2.node_id) ||
        (input1.type == "UpdateStudentOther" && (input2.type == "AddStudentForce" || input2.type == "UpdateStudentIDCascade")) ||

        (input1.type == "AddInstructorForce" && input2.type == "AddInstructorForce" && input1.node_id < input2.node_id) ||
        (input1.type == "AddInstructorForce" && (input2.type == "DeleteInstructorCascade" ||
                                                 input2.type == "UpdateInstructorIDCascade" ||
                                                 input2.type == "DeleteDepartmentNoAction")) ||
        (input1.type == "UpdateInstructorIDCascade" && input2.type == "UpdateInstructorIDCascade" && input1.node_id < input2.node_id) ||
        (input1.type == "UpdateInstructorOther" && input2.type == "UpdateInstructorOther" && input1.node_id < input2.node_id) ||
        (input1.type == "UpdateInstructorOther" && (input2.type == "AddInstructorForce" ||
                                                    input2.type == "UpdateInstructorIDCascade" ||
                                                    input2.type == "DeleteDepartmentNoAction")) ||

        (input1.type == "AddCourseForce" && input2.type == "AddCourseForce" && input1.node_id < input2.node_id) ||
        (input1.type == "AddCourseForce" && (input2.type == "DeleteCourseCascade" ||
                                             input2.type == "UpdateCourseIDCascade" ||
                                             input2.type == "DeleteDepartmentNoAction")) ||
        (input1.type == "UpdateCourseIDCascade" && input2.type == "UpdateCourseIDCascade" && input1.node_id < input2.node_id) ||
        (input1.type == "UpdateCourseNameForce" && input2.type == "UpdateCourseNameForce" && input1.node_id < input2.node_id) ||
        (input1.type == "UpdateCourseNameForce" && (input2.type == "AddCourseForce" || input2.type == "UpdateCourseIDCascade")) ||
        (input1.type == "UpdateCourseOther" && input2.type == "UpdateCourseOther" && input1.node_id < input2.node_id) ||
        (input1.type == "UpdateCourseOther" && (input2.type == "AddCourseForce" ||
                                                input2.type == "UpdateCourseIDCascade" ||
                                                input2.type == "DeleteDepartmentNoAction")) ||

        (input1.type == "AddDepartment" && input2.type == "DeleteDepartmentNoAction") ||

        (input1.type == "Enroll" && (input2.type == "DeleteStudentCascade" ||
                                     input2.type == "UpdateStudentIDCascade" ||
                                     input2.type == "DeleteCourseCascade" ||
                                     input2.type == "UpdateCourseIDCascade" ||
                                     input2.type == "WithdrawEnrollment")) ||

        (input1.type == "Teach" && (input2.type == "DeleteInstructorCascade" ||
                                    input2.type == "UpdateInstructorIDCascade" ||
                                    input2.type == "DeleteCourseCascade" ||
                                    input2.type == "UpdateCourseIDCascade" ||
                                    input2.type == "WithdrawTeaching")))
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

      // defaults
      call.value1 = 0;
      call.value2 = 0;
      call.value3 = 0;
      call.value4 = 0;

      if (call.type != "Read")
      {
        call_id_counter++;
        call.call_id = call_id_counter;

        if (call.type == "Enroll" || call.type == "Teach" || call.type == "WithdrawEnrollment" || call.type == "WithdrawTeaching" ||
            call.type == "UpdateStudentIDCascade" || call.type == "UpdateInstructorIDCascade" || call.type == "UpdateCourseIDCascade")
        {
          std::getline(iss, temp, '-');
          call.value1 = std::stoi(temp);
          iss >> call.value2;
        }
        else if (call.type == "AddStudentForce" || call.type == "UpdateStudentOther")
        {
          iss >> call.value1;
          iss >> call.value2;
          iss >> call.value3;
        }
        else if (call.type == "AddInstructorForce" || call.type == "UpdateInstructorOther")
        {
          iss >> call.value1;
          iss >> call.value2;
          iss >> call.value3;
          iss >> call.value4;
        }
        else if (call.type == "AddCourseForce")
        {
          iss >> call.value1;
          iss >> call.value2;
          iss >> call.value3;
          iss >> call.value4;
        }
        else if (call.type == "UpdateCourseOther")
        {
          iss >> call.value1;
          iss >> call.value2;
          iss >> call.value3;
        }
        else if (call.type == "UpdateCourseNameForce")
        {
          iss >> call.value1;
          iss >> call.value2;
        }
        else
        {
          // single-arg ops: DeleteStudentCascade, DeleteInstructorCascade, DeleteCourseCascade,
          // AddDepartment, DeleteDepartmentNoAction, etc.
          iss >> call.value1;
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
    if (call.type == "AddStudentForce")
    {
      ComplexCoursewareState::StudentRow row;
      row.first_name = call.value2;
      row.last_name = call.value3;
      current_state.students[call.value1] = row;
    }
    else if (call.type == "DeleteStudentCascade")
    {
      const int student_id = call.value1;

      current_state.students.erase(student_id);

      for (auto it = current_state.enroll.begin(); it != current_state.enroll.end();)
      {
        if (it->second == student_id)
          it = current_state.enroll.erase(it);
        else
          ++it;
      }
    }
    else if (call.type == "UpdateStudentIDCascade")
    {
      const int old_id = call.value1;
      const int new_id = call.value2;

      auto itS = current_state.students.find(old_id);
      if (itS != current_state.students.end())
      {
        ComplexCoursewareState::StudentRow row = itS->second;
        current_state.students.erase(itS);
        current_state.students[new_id] = row;
      }

      std::unordered_set<std::pair<int, int>, pair_hash> new_enroll;
      for (const auto &p : current_state.enroll)
      {
        if (p.second == old_id)
          new_enroll.insert(std::make_pair(p.first, new_id));
        else
          new_enroll.insert(p);
      }
      current_state.enroll.swap(new_enroll);
    }
    else if (call.type == "UpdateStudentOther")
    {
      const int student_id = call.value1;

      auto itS = current_state.students.find(student_id);
      if (itS != current_state.students.end())
      {
        itS->second.first_name = call.value2;
        itS->second.last_name = call.value3;
      }
    }
    else if (call.type == "AddInstructorForce")
    {
      ComplexCoursewareState::InstructorRow row;
      row.first_name = call.value2;
      row.last_name = call.value3;
      row.department = call.value4;
      current_state.instructors[call.value1] = row;
    }
    else if (call.type == "DeleteInstructorCascade")
    {
      const int instructor_id = call.value1;

      current_state.instructors.erase(instructor_id);

      for (auto it = current_state.teach.begin(); it != current_state.teach.end();)
      {
        if (it->second == instructor_id)
          it = current_state.teach.erase(it);
        else
          ++it;
      }
    }
    else if (call.type == "UpdateInstructorIDCascade")
    {
      const int old_id = call.value1;
      const int new_id = call.value2;

      auto itI = current_state.instructors.find(old_id);
      if (itI != current_state.instructors.end())
      {
        ComplexCoursewareState::InstructorRow row = itI->second;
        current_state.instructors.erase(itI);
        current_state.instructors[new_id] = row;
      }

      std::unordered_set<std::pair<int, int>, pair_hash> new_teach;
      for (const auto &p : current_state.teach)
      {
        if (p.second == old_id)
          new_teach.insert(std::make_pair(p.first, new_id));
        else
          new_teach.insert(p);
      }
      current_state.teach.swap(new_teach);
    }
    else if (call.type == "UpdateInstructorOther")
    {
      const int instructor_id = call.value1;

      auto itI = current_state.instructors.find(instructor_id);
      if (itI != current_state.instructors.end())
      {
        itI->second.first_name = call.value2;
        itI->second.last_name = call.value3;
        itI->second.department = call.value4;
      }
    }
    else if (call.type == "AddCourseForce")
    {
      const int course_id = call.value1;
      const int course_name = call.value2;

      // If another course already has the same course_name, delete that tuple (cascade its links)
      auto itIdx = current_state.course_name_index.find(course_name);
      if (itIdx != current_state.course_name_index.end() && itIdx->second != course_id)
      {
        const int old_course_id = itIdx->second;

        for (auto it = current_state.enroll.begin(); it != current_state.enroll.end();)
        {
          if (it->first == old_course_id)
            it = current_state.enroll.erase(it);
          else
            ++it;
        }
        for (auto it = current_state.teach.begin(); it != current_state.teach.end();)
        {
          if (it->first == old_course_id)
            it = current_state.teach.erase(it);
          else
            ++it;
        }

        current_state.courses.erase(old_course_id);
      }

      // If this course_id already existed, clear its old name->id index (if present)
      auto itC = current_state.courses.find(course_id);
      if (itC != current_state.courses.end())
      {
        const int old_name = itC->second.course_name;
        auto itOldIdx = current_state.course_name_index.find(old_name);
        if (itOldIdx != current_state.course_name_index.end() && itOldIdx->second == course_id)
          current_state.course_name_index.erase(itOldIdx);
      }

      // Overwrite name index for course_name to point to this course_id
      current_state.course_name_index[course_name] = course_id;

      ComplexCoursewareState::CourseRow row;
      row.course_name = call.value2;
      row.semester = call.value3;
      row.department = call.value4;
      current_state.courses[course_id] = row;
    }
    else if (call.type == "DeleteCourseCascade")
    {
      const int course_id = call.value1;

      auto itC = current_state.courses.find(course_id);
      if (itC != current_state.courses.end())
      {
        const int name = itC->second.course_name;
        auto itIdx = current_state.course_name_index.find(name);
        if (itIdx != current_state.course_name_index.end() && itIdx->second == course_id)
          current_state.course_name_index.erase(itIdx);

        current_state.courses.erase(itC);
      }

      for (auto it = current_state.enroll.begin(); it != current_state.enroll.end();)
      {
        if (it->first == course_id)
          it = current_state.enroll.erase(it);
        else
          ++it;
      }
      for (auto it = current_state.teach.begin(); it != current_state.teach.end();)
      {
        if (it->first == course_id)
          it = current_state.teach.erase(it);
        else
          ++it;
      }
    }
    else if (call.type == "UpdateCourseIDCascade")
    {
      const int old_id = call.value1;
      const int new_id = call.value2;

      auto itC = current_state.courses.find(old_id);
      if (itC != current_state.courses.end())
      {
        ComplexCoursewareState::CourseRow row = itC->second;
        current_state.courses.erase(itC);
        current_state.courses[new_id] = row;

        current_state.course_name_index[row.course_name] = new_id;
      }

      std::unordered_set<std::pair<int, int>, pair_hash> new_enroll;
      for (const auto &p : current_state.enroll)
      {
        if (p.first == old_id)
          new_enroll.insert(std::make_pair(new_id, p.second));
        else
          new_enroll.insert(p);
      }
      current_state.enroll.swap(new_enroll);

      std::unordered_set<std::pair<int, int>, pair_hash> new_teach;
      for (const auto &p : current_state.teach)
      {
        if (p.first == old_id)
          new_teach.insert(std::make_pair(new_id, p.second));
        else
          new_teach.insert(p);
      }
      current_state.teach.swap(new_teach);
    }
    else if (call.type == "UpdateCourseNameForce")
    {
      const int course_id = call.value1;
      const int new_name = call.value2;

      // If another course already has new_name, delete that tuple (cascade its links)
      auto itIdx = current_state.course_name_index.find(new_name);
      if (itIdx != current_state.course_name_index.end() && itIdx->second != course_id)
      {
        const int old_course_id = itIdx->second;

        for (auto it = current_state.enroll.begin(); it != current_state.enroll.end();)
        {
          if (it->first == old_course_id)
            it = current_state.enroll.erase(it);
          else
            ++it;
        }
        for (auto it = current_state.teach.begin(); it != current_state.teach.end();)
        {
          if (it->first == old_course_id)
            it = current_state.teach.erase(it);
          else
            ++it;
        }

        current_state.courses.erase(old_course_id);
      }

      auto itC = current_state.courses.find(course_id);
      if (itC != current_state.courses.end())
      {
        const int old_name = itC->second.course_name;
        auto itOldIdx = current_state.course_name_index.find(old_name);
        if (itOldIdx != current_state.course_name_index.end() && itOldIdx->second == course_id)
          current_state.course_name_index.erase(itOldIdx);

        itC->second.course_name = new_name;
        current_state.course_name_index[new_name] = course_id;
      }
      else
      {
        // If course doesn't exist, still keep the unique-name index consistent
        current_state.course_name_index[new_name] = course_id;
      }
    }
    else if (call.type == "UpdateCourseOther")
    {
      const int course_id = call.value1;

      auto itC = current_state.courses.find(course_id);
      if (itC != current_state.courses.end())
      {
        itC->second.semester = call.value2;
        itC->second.department = call.value3;
      }
    }
    else if (call.type == "AddDepartment")
    {
      current_state.departments.insert(call.value1);
    }
    else if (call.type == "DeleteDepartmentNoAction")
    {
      current_state.departments.erase(call.value1);
    }
    else if (call.type == "Enroll")
    {
      current_state.enroll.insert(std::make_pair(call.value1, call.value2));
    }
    else if (call.type == "WithdrawEnrollment")
    {
      current_state.enroll.erase(std::make_pair(call.value1, call.value2));
    }
    else if (call.type == "Teach")
    {
      current_state.teach.insert(std::make_pair(call.value1, call.value2));
    }
    else if (call.type == "WithdrawTeaching")
    {
      current_state.teach.erase(std::make_pair(call.value1, call.value2));
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
    if (call.type == "AddStudentForce")
    {
      ComplexCoursewareState::StudentRow row;
      row.first_name = call.value2;
      row.last_name = call.value3;
      stable_state.students[call.value1] = row;
    }
    else if (call.type == "DeleteStudentCascade")
    {
      const int student_id = call.value1;

      stable_state.students.erase(student_id);

      for (auto it = stable_state.enroll.begin(); it != stable_state.enroll.end();)
      {
        if (it->second == student_id)
          it = stable_state.enroll.erase(it);
        else
          ++it;
      }
    }
    else if (call.type == "UpdateStudentIDCascade")
    {
      const int old_id = call.value1;
      const int new_id = call.value2;

      auto itS = stable_state.students.find(old_id);
      if (itS != stable_state.students.end())
      {
        ComplexCoursewareState::StudentRow row = itS->second;
        stable_state.students.erase(itS);
        stable_state.students[new_id] = row;
      }

      std::unordered_set<std::pair<int, int>, pair_hash> new_enroll;
      for (const auto &p : stable_state.enroll)
      {
        if (p.second == old_id)
          new_enroll.insert(std::make_pair(p.first, new_id));
        else
          new_enroll.insert(p);
      }
      stable_state.enroll.swap(new_enroll);
    }
    else if (call.type == "UpdateStudentOther")
    {
      const int student_id = call.value1;

      auto itS = stable_state.students.find(student_id);
      if (itS != stable_state.students.end())
      {
        itS->second.first_name = call.value2;
        itS->second.last_name = call.value3;
      }
    }
    else if (call.type == "AddInstructorForce")
    {
      ComplexCoursewareState::InstructorRow row;
      row.first_name = call.value2;
      row.last_name = call.value3;
      row.department = call.value4;
      stable_state.instructors[call.value1] = row;
    }
    else if (call.type == "DeleteInstructorCascade")
    {
      const int instructor_id = call.value1;

      stable_state.instructors.erase(instructor_id);

      for (auto it = stable_state.teach.begin(); it != stable_state.teach.end();)
      {
        if (it->second == instructor_id)
          it = stable_state.teach.erase(it);
        else
          ++it;
      }
    }
    else if (call.type == "UpdateInstructorIDCascade")
    {
      const int old_id = call.value1;
      const int new_id = call.value2;

      auto itI = stable_state.instructors.find(old_id);
      if (itI != stable_state.instructors.end())
      {
        ComplexCoursewareState::InstructorRow row = itI->second;
        stable_state.instructors.erase(itI);
        stable_state.instructors[new_id] = row;
      }

      std::unordered_set<std::pair<int, int>, pair_hash> new_teach;
      for (const auto &p : stable_state.teach)
      {
        if (p.second == old_id)
          new_teach.insert(std::make_pair(p.first, new_id));
        else
          new_teach.insert(p);
      }
      stable_state.teach.swap(new_teach);
    }
    else if (call.type == "UpdateInstructorOther")
    {
      const int instructor_id = call.value1;

      auto itI = stable_state.instructors.find(instructor_id);
      if (itI != stable_state.instructors.end())
      {
        itI->second.first_name = call.value2;
        itI->second.last_name = call.value3;
        itI->second.department = call.value4;
      }
    }
    else if (call.type == "AddCourseForce")
    {
      const int course_id = call.value1;
      const int course_name = call.value2;

      // If another course already has the same course_name, delete that tuple (cascade its links)
      auto itIdx = stable_state.course_name_index.find(course_name);
      if (itIdx != stable_state.course_name_index.end() && itIdx->second != course_id)
      {
        const int old_course_id = itIdx->second;

        for (auto it = stable_state.enroll.begin(); it != stable_state.enroll.end();)
        {
          if (it->first == old_course_id)
            it = stable_state.enroll.erase(it);
          else
            ++it;
        }
        for (auto it = stable_state.teach.begin(); it != stable_state.teach.end();)
        {
          if (it->first == old_course_id)
            it = stable_state.teach.erase(it);
          else
            ++it;
        }

        stable_state.courses.erase(old_course_id);
      }

      // If this course_id already existed, clear its old name->id index (if present)
      auto itC = stable_state.courses.find(course_id);
      if (itC != stable_state.courses.end())
      {
        const int old_name = itC->second.course_name;
        auto itOldIdx = stable_state.course_name_index.find(old_name);
        if (itOldIdx != stable_state.course_name_index.end() && itOldIdx->second == course_id)
          stable_state.course_name_index.erase(itOldIdx);
      }

      // Overwrite name index for course_name to point to this course_id
      stable_state.course_name_index[course_name] = course_id;

      ComplexCoursewareState::CourseRow row;
      row.course_name = call.value2;
      row.semester = call.value3;
      row.department = call.value4;
      stable_state.courses[course_id] = row;
    }
    else if (call.type == "DeleteCourseCascade")
    {
      const int course_id = call.value1;

      auto itC = stable_state.courses.find(course_id);
      if (itC != stable_state.courses.end())
      {
        const int name = itC->second.course_name;
        auto itIdx = stable_state.course_name_index.find(name);
        if (itIdx != stable_state.course_name_index.end() && itIdx->second == course_id)
          stable_state.course_name_index.erase(itIdx);

        stable_state.courses.erase(itC);
      }

      for (auto it = stable_state.enroll.begin(); it != stable_state.enroll.end();)
      {
        if (it->first == course_id)
          it = stable_state.enroll.erase(it);
        else
          ++it;
      }
      for (auto it = stable_state.teach.begin(); it != stable_state.teach.end();)
      {
        if (it->first == course_id)
          it = stable_state.teach.erase(it);
        else
          ++it;
      }
    }
    else if (call.type == "UpdateCourseIDCascade")
    {
      const int old_id = call.value1;
      const int new_id = call.value2;

      auto itC = stable_state.courses.find(old_id);
      if (itC != stable_state.courses.end())
      {
        ComplexCoursewareState::CourseRow row = itC->second;
        stable_state.courses.erase(itC);
        stable_state.courses[new_id] = row;

        stable_state.course_name_index[row.course_name] = new_id;
      }

      std::unordered_set<std::pair<int, int>, pair_hash> new_enroll;
      for (const auto &p : stable_state.enroll)
      {
        if (p.first == old_id)
          new_enroll.insert(std::make_pair(new_id, p.second));
        else
          new_enroll.insert(p);
      }
      stable_state.enroll.swap(new_enroll);

      std::unordered_set<std::pair<int, int>, pair_hash> new_teach;
      for (const auto &p : stable_state.teach)
      {
        if (p.first == old_id)
          new_teach.insert(std::make_pair(new_id, p.second));
        else
          new_teach.insert(p);
      }
      stable_state.teach.swap(new_teach);
    }
    else if (call.type == "UpdateCourseNameForce")
    {
      const int course_id = call.value1;
      const int new_name = call.value2;

      // If another course already has new_name, delete that tuple (cascade its links)
      auto itIdx = stable_state.course_name_index.find(new_name);
      if (itIdx != stable_state.course_name_index.end() && itIdx->second != course_id)
      {
        const int old_course_id = itIdx->second;

        for (auto it = stable_state.enroll.begin(); it != stable_state.enroll.end();)
        {
          if (it->first == old_course_id)
            it = stable_state.enroll.erase(it);
          else
            ++it;
        }
        for (auto it = stable_state.teach.begin(); it != stable_state.teach.end();)
        {
          if (it->first == old_course_id)
            it = stable_state.teach.erase(it);
          else
            ++it;
        }

        stable_state.courses.erase(old_course_id);
      }

      auto itC = stable_state.courses.find(course_id);
      if (itC != stable_state.courses.end())
      {
        const int old_name = itC->second.course_name;
        auto itOldIdx = stable_state.course_name_index.find(old_name);
        if (itOldIdx != stable_state.course_name_index.end() && itOldIdx->second == course_id)
          stable_state.course_name_index.erase(itOldIdx);

        itC->second.course_name = new_name;
        stable_state.course_name_index[new_name] = course_id;
      }
      else
      {
        // If course doesn't exist, still keep the unique-name index consistent
        // stable_state.course_name_index[new_name] = course_id;
      }
    }
    else if (call.type == "UpdateCourseOther")
    {
      const int course_id = call.value1;

      auto itC = stable_state.courses.find(course_id);
      if (itC != stable_state.courses.end())
      {
        itC->second.semester = call.value2;
        itC->second.department = call.value3;
      }
    }
    else if (call.type == "AddDepartment")
    {
      stable_state.departments.insert(call.value1);
    }
    else if (call.type == "DeleteDepartmentNoAction")
    {
      stable_state.departments.erase(call.value1);
    }
    else if (call.type == "Enroll")
    {
      stable_state.enroll.insert(std::make_pair(call.value1, call.value2));
    }
    else if (call.type == "WithdrawEnrollment")
    {
      stable_state.enroll.erase(std::make_pair(call.value1, call.value2));
    }
    else if (call.type == "Teach")
    {
      stable_state.teach.insert(std::make_pair(call.value1, call.value2));
    }
    else if (call.type == "WithdrawTeaching")
    {
      stable_state.teach.erase(std::make_pair(call.value1, call.value2));
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