// File: ecros-set.cpp
#pragma once
#include "replicated-object.hpp"

// ---------------------------------------------------------------------------
// Set-specific Ordana analysis stubs (ECRO Section 4.2)
// ---------------------------------------------------------------------------

inline bool
seqCommutative(const Call *a, const Call *b)
{
    return ((a->type == "Add" && b->type == "Add") ||
        (a->type == "Remove" && b->type == "Remove"));
}

inline bool commutative(const Call *a, const Call *b)
{
    return seqCommutative(a, b);
}

inline char resolution(const Call *a, const Call *b)
{
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
// Set: holds state σ and implements apply/recompute for Set use-case
// ---------------------------------------------------------------------------
class Set
{
public:
    // The set state
    std::unordered_set<int> elements;
    bool send_ack;
    std::shared_ptr<std::vector<Call>> send_ack_call_list = std::make_shared<std::vector<Call>>();
    std::atomic<int> waittobestable;
    std::atomic<int> ackindex;
    Call send_ack_call;
    std::mutex pendMtx;
    std::map<int, Call> pendingLocalCalls; // call_id → Call
    mutable std::mutex setMtx;
    std::set<std::pair<int, int>> applied;

    Set()
    {
        // Initialize the set state
        elements.clear();
        send_ack = false;
        waittobestable.store(0);
        ackindex.store(0);
    };

    // Called by Handler::setVars to propagate node/node_count
    void setVars(int id, int nodes)
    {
        // No additional per-set initialization needed here
        for (int i = 0; i < 50; ++i)
            elements.insert(i);
    }

    // Apply a single Call to this replica's set state
    void applyCall(Call *c)
    {
        if (c->type == "Add")
        {
            elements.insert(c->value1);
        }
        else if (c->type == "Remove" && !elements.empty())
        {
            elements.erase(c->value1);
        }
    }
    bool shouldSkip(Call &call)
    {
        if (call.type == "Remove")
        {
            // std::lock_guard<std::mutex> lock(setMtx);
            return elements.empty();
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

            if (call->type == "Add")
            {
                elements.insert(call->value1);
            }
            else if (call->type == "Remove" && !elements.empty())
            {
                elements.erase(call->value1);
            }

            applied.insert(unique_id);
        }
    }

    enum SetEnum
    {
        Add,
        Remove,
        Read
    };

    std::string setToString(int set)
    {
        switch (static_cast<SetEnum>(set))
        {
        case Add:
            return "Add";
        case Remove:
            return "Remove";
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
            call.type = setToString(tmp_op);
            // std::cout << "readBenchFile--" << "call.type :" << call.type << std::endl;
            if (call.type != "Read")
            {

                call_id_counter++;
                call.call_id = call_id_counter;
                if (call.type == "Add" || call.type == "Remove")
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
    
    bool checkValidcall(Call call)
    {
        if (call.type == "Add" || call.type == "Remove")
            true;
        else
            false;
    }
};
