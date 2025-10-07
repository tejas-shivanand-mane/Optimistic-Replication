// File: ecros-courseware.cpp
#pragma once
#include "replicated-object.hpp"

// ---------------------------------------------------------------------------
// Stack-specific Ordana analysis stubs (ECRO Section 4.2)
// ---------------------------------------------------------------------------

inline bool
seqCommutative(const Call *a, const Call *b)
{
    if ((a->type == "AddCourse" && b->type == "AddCourse") ||
        (a->type == "AddCourse" && b->type == "AddStudent") ||
        (a->type == "AddCourse" && b->type == "DeleteStudent") ||
        (a->type == "AddCourse" && b->type == "Enroll"))
        return true;

    if ((a->type == "AddStudent" && b->type == "AddCourse") ||
        (a->type == "AddStudent" && b->type == "AddStudent") ||
        (a->type == "AddStudent" && b->type == "DeleteCourse") ||
        (a->type == "AddStudent" && b->type == "Enroll"))
        return true;

    if ((a->type == "DeleteCourse" && b->type == "AddStudent") ||
        (a->type == "DeleteCourse" && b->type == "DeleteCourse") ||
        (a->type == "DeleteCourse" && b->type == "DeleteStudent") ||
        (a->type == "DeleteCourse" && b->type == "Enroll"))
        return true;

    if ((a->type == "Enroll" && b->type == "AddCourse") ||
        (a->type == "Enroll" && b->type == "AddStudent") ||
        (a->type == "Enroll" && b->type == "DeleteCourse") ||
        (a->type == "Enroll" && b->type == "DeleteStudent") ||
        (a->type == "Enroll" && b->type == "Enroll"))
        return true;

    return false; // Default: not commutative
}

inline bool commutative(const Call *a, const Call *b)
{
    return seqCommutative(a, b);
}

inline char resolution(const Call *a, const Call *b)
{
    if ((a->type == "DeleteCourse" || a->type == "DeleteStudent") && b->type == "Enroll")
        return '<';

    if (a->type == "Enroll" && (b->type == "DeleteCourse" || b->type == "DeleteStudent"))
        return '>';
    return '⊤';
}

inline char idcmp(const Call *a, const Call *b)
{
    // Deterministic tie-break: node_id, then call_id
    if (a->node_id < b->node_id)
        return '<';
    if (a->node_id > b->node_id)
        return '>';
    if (a->call_id < b->call_id)
        return '<';
    if (a->call_id > b->call_id)
        return '>';
    return '⊤';
}

// ---------------------------------------------------------------------------
//  Courseware: holds state σ and implements apply/recompute for courseware use-case
// ---------------------------------------------------------------------------
class Courseware
{
public:
    
    std::unordered_set<int> student_ids;
    std::unordered_set<int> course_ids;
    std::unordered_set<std::pair<int, int>, pair_hash> enroll;
    bool send_ack;
    std::shared_ptr<std::vector<Call>> send_ack_call_list = std::make_shared<std::vector<Call>>();
    std::atomic<int> waittobestable;
    std::atomic<int> ackindex;
    Call send_ack_call;
    std::mutex pendMtx;
    std::map<int, Call> pendingLocalCalls; // call_id → Call
    std::set<std::pair<int, int>> applied;

    Courseware()
    {
        // Initialize the courseware state
        student_ids.clear();
        course_ids.clear();
        send_ack = false;

        // Allocate memory for the array on the heap
        waittobestable.store(0);
        ackindex.store(0);
    };

    // Called by Handler::setVars to propagate node/node_count
    void setVars(int id, int nodes)
    {
        // No additional per-courseware initialization needed here
        for (int i = 0; i < 50; ++i)
        {
            student_ids.insert(i);
            course_ids.insert(i);
        }
    }

    // Apply a single Call to this replica's courseware state
    void applyCall(Call *c)
    {
        std::pair<int, int> unique_id = {c->call_id, c->node_id};

        if (applied.count(unique_id))
            return; // Already applied

        if (c->type == "AddCourse")
        {
            course_ids.insert(c->value1);
        }
        else if (c->type == "DeleteCourse" && !course_ids.empty())
        {
            // Only delete if there are projects
            // If no projects, we skip this call
            // std::lock_guard<std::mutex> lock(projectMtx);
            course_ids.erase(c->value1);
        }
        else if (c->type == "Enroll")
        {
            enroll.insert({c->value1, c->value2});
        }
        else if (c->type == "AddStudent")
        {
            student_ids.insert(c->value1);
        }
        else if (c->type == "DeleteStudent" && !student_ids.empty())
        {
            // Only delete if there are employees
            // If no employees, we skip this call
            // std::lock_guard<std::mutex> lock(projectMtx);
            student_ids.erase(c->value1);
        }

        applied.insert(unique_id);
    }
    bool shouldSkip(Call &call)
    {
        if (call.type == "DeleteCourse")
        {
            // std::lock_guard<std::mutex> lock(projectMtx);
            return course_ids.empty();
        }
        else if ("DeleteStudent")
        {
            return student_ids.empty();
        }
        return false;
    }
    // Replay the entire topoOrder to rebuild state from σ₀
    void recomputeState(const std::vector<Call *> &order)
    {
        for (Call *call : order)
        {
            std::pair<int, int> unique_id = {call->call_id, call->node_id};

            if (applied.count(unique_id))
                continue; // Already applied

            if (call->type == "AddCourse")
            {
                course_ids.insert(call->value1);
            }
            else if (call->type == "DeleteCourse" && !course_ids.empty())

            {
                // Only delete if there are projects
                // If no projects, we skip this call
                // std::lock_guard<std::mutex> lock(projectMtx);
                course_ids.erase(call->value1);
            }
            else if (call->type == "Enroll")
            {
                enroll.insert({call->value1, call->value2});
            }
            else if (call->type == "AddStudent")
            {
                student_ids.insert(call->value1);
            }
            else if (call->type == "DeleteStudent" && !student_ids.empty())
            {
                // Only delete if there are employees
                // If no employees, we skip this call
                // std::lock_guard<std::mutex> lock(projectMtx);
                student_ids.erase(call->value1);
            }

            applied.insert(unique_id);
        }
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

  bool checkValidcall(Call call)
  {
    if (call.type == "AddCourse" || call.type == "DeleteCourse" || call.type == "Enroll" || call.type == "AddStudent" || call.type == "DeleteStudent")
      true;
    else
      false;
  }
};
