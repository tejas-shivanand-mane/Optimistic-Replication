#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <unordered_set>
#include <mutex>
#include <sstream>
#include <atomic>
#include <list>
#include <unordered_map>
#include <queue>
#include <functional>
#include <set>
#include <map>
#include <stack>


class Call
{
public:
    std::string type;
    int value1;
    int value2;
    int node_id;
    int call_id;
    bool stable;
    std::vector<int> call_vector_clock; // Add this line
    bool applied;

    // Default constructor
    Call() : type(""), value1(0), value2(0), node_id(0), call_id(0), stable(false), call_vector_clock(8, 0), applied(false) {}

    // Parameterized constructor
    Call(std::string t, int v1, int v2, int n_id = 0, int c_id = 0, bool s = false, bool p = false)
        : type(t), value1(v1), value2(v2), node_id(n_id), call_id(c_id), stable(s), call_vector_clock(8, 0), applied(p) {}
};

class DirectedGraph {
public:
    std::unordered_map<std::string, std::list<std::string>> adjList;

    void addEdge(const std::string& src, const std::string& dest) {
        adjList[src].push_back(dest);
    }

    bool isReachable(const std::string& src, const std::string& dest) {
        if (src == dest) return false; // Return false if source is equal to destination
        std::unordered_map<std::string, bool> visited;
        return dfs(src, dest, visited);
    }

private:
    bool dfs(const std::string& src, const std::string& dest, std::unordered_map<std::string, bool>& visited) {
        if (src == dest) return true;
        visited[src] = true;

        for (const auto& neighbor : adjList[src]) {
            if (!visited[neighbor] && dfs(neighbor, dest, visited)) {
                return true;
            }
        }
        return false;
    }
};



struct pair_hash {
    inline std::size_t operator()(const std::pair<int, int>& v) const {
        std::hash<int> hasher;
        return hasher(v.first) * 31 + hasher(v.second);
    }
};


