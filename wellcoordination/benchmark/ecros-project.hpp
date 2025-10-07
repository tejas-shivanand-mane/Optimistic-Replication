// File: ecros-project.cpp
#pragma once
#include "replicated-object.hpp"

// ---------------------------------------------------------------------------
// Stack-specific Ordana analysis stubs (ECRO Section 4.2)
// ---------------------------------------------------------------------------

inline bool
seqCommutative(const Call *a, const Call *b)
{
    // Employee-project domain: allow add/delete pairs to commute
    if ((a->type == "AddEmployee" && b->type == "AaddEmployee") ||
        (a->type == "AddEmployee" && b->type == "AddProject") ||
        (a->type == "AddEmployee" && b->type == "DeleteProject") ||
        (a->type == "AddEmployee" && b->type == "WorksOn"))
        return true;

    if ((a->type == "AddProject" && b->type == "AddProject") ||
        (a->type == "AddProject" && b->type == "AddEmployee") ||
        (a->type == "AddProject" && b->type == "DeleteEmployee") ||
        (a->type == "AddProject" && b->type == "WorksOn"))
        return true;

    if ((a->type == "DeleteEmployee" && b->type == "DeleteEmployee") ||
        (a->type == "DeleteEmployee" && b->type == "DeleteProject") ||
        (a->type == "DeleteEmployee" && b->type == "AddProject"))
        return true;

    if ((a->type == "DeleteProject" && b->type == "DeleteProject") ||
        (a->type == "DeleteProject" && b->type == "DeleteEmployee") ||
        (a->type == "DeleteProject" && b->type == "AddEmployee"))
        return true;

    if ((a->type == "WorksOn" && b->type == "WorksOn") ||
        (a->type == "WorksOn" && b->type == "AddEmployee") ||
        (a->type == "WorksOn" && b->type == "AddProject"))
        return true;

    return false; // Default: not commutative
}

inline bool commutative(const Call *a, const Call *b)
{
    return seqCommutative(a, b);
}

inline char resolution(const Call *a, const Call *b)
{
    if ((a->type == "DeleteEmployee" && b->type == "WorksOn") ||
        (a->type == "DeleteProject" && b->type == "WorksOn"))
        return '<';

    if ((a->type == "WorksOn" && b->type == "DeleteEmployee") ||
        (a->type == "WorksOn" && b->type == "DeleteProject"))
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
//  Project: holds state σ and implements apply/recompute for project use-case
// ---------------------------------------------------------------------------
class Project
{
public:
    // The project state
    std::unordered_set<int> employee_ids;
    std::unordered_set<int> project_ids;
    std::unordered_set<std::pair<int, int>, pair_hash> works;
    bool send_ack;
    std::shared_ptr<std::vector<Call>> send_ack_call_list = std::make_shared<std::vector<Call>>();
    std::atomic<int> waittobestable;
    std::atomic<int> ackindex;
    Call send_ack_call;
    std::mutex pendMtx;
    std::map<int, Call> pendingLocalCalls; // call_id → Call
    mutable std::mutex projectMtx;
    std::set<std::pair<int, int>> applied;

    Project()
    {
        // Initialize the project state
        employee_ids.clear();
        project_ids.clear();
        send_ack = false;

        // Allocate memory for the array on the heap
        waittobestable.store(0);
        ackindex.store(0);
    };

    // Called by Handler::setVars to propagate node/node_count
    void setVars(int id, int nodes)
    {
        // No additional per-project initialization needed here
        for (int i = 0; i < 50; ++i)
        {
            employee_ids.insert(i);
            project_ids.insert(i);
        }
    }

    // Apply a single Call to this replica's project state
    void applyCall(Call *c)
    {
        std::pair<int, int> unique_id = {c->call_id, c->node_id};

        if (applied.count(unique_id))
            return; // Already applied

        if (c->type == "AddProject")
        {
            project_ids.insert(c->value1);
        }
        else if (c->type == "DeleteProject" && !project_ids.empty())
        {
            // Only delete if there are projects
            // If no projects, we skip this call
            // std::lock_guard<std::mutex> lock(projectMtx);
            project_ids.erase(c->value1);
        }
        else if (c->type == "WorksOn")
        {
            works.insert({c->value1, c->value2});
        }
        else if (c->type == "AddEmployee")
        {
            employee_ids.insert(c->value1);
        }
        else if (c->type == "DeleteEmployee" && !employee_ids.empty())
        {
            // Only delete if there are employees
            // If no employees, we skip this call
            // std::lock_guard<std::mutex> lock(projectMtx);
            employee_ids.erase(c->value1);
        }

        applied.insert(unique_id);
    }
    bool shouldSkip(Call &call)
    {
        if (call.type == "DeleteProject")
        {
            // std::lock_guard<std::mutex> lock(projectMtx);
            return project_ids.empty();
        }
        else if ("DeleteEmployee")
        {
            return employee_ids.empty();
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

            if (call->type == "AddProject")
            {
                project_ids.insert(call->value1);
            }
            else if (call->type == "DeleteProject" && !project_ids.empty())

            {
                // Only delete if there are projects
                // If no projects, we skip this call
                // std::lock_guard<std::mutex> lock(projectMtx);
                project_ids.erase(call->value1);
            }
            else if (call->type == "WorksOn")
            {
                works.insert({call->value1, call->value2});
            }
            else if (call->type == "AddEmployee")
            {
                employee_ids.insert(call->value1);
            }
            else if (call->type == "DeleteEmployee" && !employee_ids.empty())
            {
                // Only delete if there are employees
                // If no employees, we skip this call
                // std::lock_guard<std::mutex> lock(projectMtx);
                employee_ids.erase(call->value1);
            }

            applied.insert(unique_id);
        }
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

    bool checkValidcall(Call call)
    {
        if (call.type == "AddProject" || call.type == "DeleteProject" || call.type == "WorksOn" || call.type == "AddEmployee" || call.type == "DeleteEmployee")
            true;
        else
            false;
    }
};
