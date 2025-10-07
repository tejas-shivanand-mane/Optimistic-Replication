// File: ecros-movie.cpp
#pragma once
#include "replicated-object.hpp"

// ---------------------------------------------------------------------------
// Stack-specific Ordana analysis stubs (ECRO Section 4.2)
// ---------------------------------------------------------------------------

inline bool
seqCommutative(const Call *a, const Call *b)
{
    if ((a->type == "AddMovie" && b->type == "AddMovie") ||
        (a->type == "AddMovie" && b->type == "AddCustomer") ||
        (a->type == "AddMovie" && b->type == "DeleteCustomer"))
        return true;

    if ((a->type == "AddCustomer" && b->type == "AddMovie") ||
        (a->type == "AddCustomer" && b->type == "AddCustomer") ||
        (a->type == "AddCustomer" && b->type == "DeleteMovie"))
        return true;

    if ((a->type == "DeleteMovie" && b->type == "AddCustomer") ||
        (a->type == "DeleteMovie" && b->type == "DeleteMovie") ||
        (a->type == "DeleteMovie" && b->type == "DeleteCustomer"))
        return true;

    if ((a->type == "DeleteCustomer" && b->type == "AddMovie") ||
        (a->type == "DeleteCustomer" && b->type == "DeleteMovie") ||
        (a->type == "DeleteCustomer" && b->type == "DeleteCustomer"))
        return true;

    return false; // Default: not commutative
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
//  Movie: holds state σ and implements apply/recompute for movie use-case
// ---------------------------------------------------------------------------
class Movie
{
public:
    // The movie state
    std::unordered_set<int> movie_ids;
    std::unordered_set<int> customer_ids;
    bool send_ack;
    std::shared_ptr<std::vector<Call>> send_ack_call_list = std::make_shared<std::vector<Call>>();
    std::atomic<int> waittobestable;
    std::atomic<int> ackindex;
    Call send_ack_call;
    std::mutex pendMtx;
    std::map<int, Call> pendingLocalCalls; // call_id → Call
    mutable std::mutex projectMtx;
    std::set<std::pair<int, int>> applied;

    Movie()
    {
        // Initialize the movie state
        movie_ids.clear();
        customer_ids.clear();
        send_ack = false;

        // Allocate memory for the array on the heap
        waittobestable.store(0);
        ackindex.store(0);
    };

    // Called by Handler::setVars to propagate node/node_count
    void setVars(int id, int nodes)
    {
        // No additional per-movie initialization needed here
        for (int i = 0; i < 50; ++i)
        {
            movie_ids.insert(i);
            customer_ids.insert(i);
        }
    }

    // Apply a single Call to this replica's movie state
    void applyCall(Call *c)
    {
        std::pair<int, int> unique_id = {c->call_id, c->node_id};

        if (applied.count(unique_id))
            return; // Already applied

        if (c->type == "AddMovie")
        {
            movie_ids.insert(c->value1);
        }
        else if (c->type == "DeleteMovie" && !movie_ids.empty())
        {
            // Only delete if there are projects
            // If no projects, we skip this call
            // std::lock_guard<std::mutex> lock(projectMtx);
            movie_ids.erase(c->value1);
        }
        else if (c->type == "AddCustomer")
        {
            customer_ids.insert(c->value1);
        }
        else if (c->type == "DeleteCustomer" && !movie_ids.empty())
        {
            // Only delete if there are employees
            // If no employees, we skip this call
            // std::lock_guard<std::mutex> lock(projectMtx);
            customer_ids.erase(c->value1);
        }

        applied.insert(unique_id);
    }
    bool shouldSkip(Call &call)
    {
        if (call.type == "DeleteCustomer")
        {
            // std::lock_guard<std::mutex> lock(projectMtx);
            return customer_ids.empty();
        }
        else if ("DeleteMovie")
        {
            return movie_ids.empty();
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

            if (call->type == "AddMovie")
            {
                movie_ids.insert(call->value1);
            }
            else if (call->type == "DeleteMovie" && !movie_ids.empty())

            {
                // Only delete if there are projects
                // If no projects, we skip this call
                // std::lock_guard<std::mutex> lock(projectMtx);
                movie_ids.erase(call->value1);
            }
            else if (call->type == "AddCustomer")
            {
                customer_ids.insert(call->value1);
            }
            else if (call->type == "DeleteCustomer" && !customer_ids.empty())
            {
                // Only delete if there are employees
                // If no employees, we skip this call
                // std::lock_guard<std::mutex> lock(projectMtx);
                customer_ids.erase(call->value1);
            }

            applied.insert(unique_id);
        }
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

    bool
    checkValidcall(Call call)
    {
        if (call.type == "AddMovie" || call.type == "DeleteMovie" || call.type == "AddCustomer" || call.type == "DeleteCustomer")
            true;
        else
            false;
    }
};
