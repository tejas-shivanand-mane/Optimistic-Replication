#include <iostream>
#include <list>
#include <stack>
#include <algorithm>  // Include this library
#define NIL -1
using namespace std;

class Graph {
    int V;    // No. of vertices
    list<int> *adj;    // A dynamic array of adjacency lists

public:
    // Constructor and destructor
    Graph(int V) { this->V = V; adj = new list<int>[V]; }
    ~Graph() { delete [] adj; }

    // functions to add and remove edges
    void addEdge(int u, int v) { adj[u].push_back(v); }
    void removeEdge(int u, int v);

    // Prints all cycles in the graph
    void printCycles(int n);

private:
    // A function used by printCycles
    void printCyclesUtil(int u, int v, int color[], int pred[], int &cycleNumber);
};

void Graph::removeEdge(int u, int v) {
    // Find v in adjacency list of u and replace it with NIL
    list<int>::iterator iv = find(adj[u].begin(), adj[u].end(), v);
    *iv = NIL;

    // Find u in adjacency list of v and replace it with NIL
    list<int>::iterator iu = find(adj[v].begin(), adj[v].end(), u);
    *iu = NIL;
}

// depth-first traversal and search for back edges.
void Graph::printCyclesUtil(int u, int v, int color[], int pred[], int &cycleNumber) {
    // insert gray edge into backbone
    if (color[u] == 2) {
        return;
    }
    if (color[u] == 1) {
        cycleNumber++;
        int cur = v;
        pred[cur] = u;
        while (cur != u) {
            cur = pred[cur];
        }
        return;
    }
    pred[u] = v;
    color[u] = 1;

    // DFS Traversal
    list<int>::iterator i;
    for (i = adj[u].begin(); i != adj[u].end(); ++i) {
        if (*i == NIL) {
            continue;
        }
        if (color[*i] == 0) {
            printCyclesUtil(*i, u, color, pred, cycleNumber);
        } else if (color[*i] == 1) {
            printCyclesUtil(*i, u, color, pred, cycleNumber);
        }
    }

    // remove vertex from path
    color[u] = 2;
}

void Graph::printCycles(int n) {
    // arrays required to color the graph, store the parent of node
    int *color = new int[n], *pred = new int[n];
    int cycleNumber = 0;  // Initialize result

    // initially all vertices are white (0)
    for (int i = 0; i < n; i++) {
        color[i] = 0;
        pred[i] = 0;
    }

    // DFS to find cycles
    for (int i = 0; i < n; i++) {
        if (color[i] == 0) {
            printCyclesUtil(i, NIL, color, pred, cycleNumber);
        }
    }

    cout << "Graph contains " << cycleNumber << " cycle(s)" << endl;
}

// Driver program to test above functions
int main() {
    Graph g1(4);
    g1.addEdge(0, 1);
    g1.addEdge(0, 2);
    g1.addEdge(1, 2);
    g1.addEdge(2, 0);
    g1.addEdge(2, 3);
    g1.addEdge(3, 3);

    g1.printCycles(4);

    g1.removeEdge(3,3);

    g1.printCycles(4);

    return 0;
}
