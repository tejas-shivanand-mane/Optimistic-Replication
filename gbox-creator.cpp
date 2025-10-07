#include <iostream>
#include <vector>
#include <algorithm>
#include <map>
#include <set>

enum Vertex {
    AddProject, 
    DeleteProject, 
    WorksOn, 
    AddEmployee
};

class Graph {
    int numVertices;
    std::vector<int>* adjLists;
    std::map<std::pair<Vertex, Vertex>, std::string> edgeOrigins;
    std::map<int, std::string> vertexNames;

public:
    Graph(int vertices) {
        numVertices = vertices;
        adjLists = new std::vector<int>[vertices];
        vertexNames[0] = "AddProject";
        vertexNames[1] = "DeleteProject";
        vertexNames[2] = "WorksOn";
        vertexNames[3] = "AddEmployee";
    }

    void addEdge(Vertex src, Vertex dest, const std::string& origin) {
        adjLists[src].push_back(dest);
        edgeOrigins[std::make_pair(src, dest)] = origin;
    }
void removeEdge(int u, int v) {
    // Find v in adjacency list of u and remove it
    adjLists[u].erase(std::remove(adjLists[u].begin(), adjLists[u].end(), v), adjLists[u].end());
}


    void reverseEdges() {
        std::vector<int>* reversedAdjLists = new std::vector<int>[numVertices];
        for(int v = 0; v < numVertices; v++) {
            for(auto x : adjLists[v]) {
                reversedAdjLists[x].push_back(v);
            }
        }
        delete[] adjLists;
        adjLists = reversedAdjLists;
    }

    void merge2Graphs(Graph& g1, const std::string& g1origin, Graph& g2, const std::string& g2origin) {
        for(int v = 0; v < numVertices; v++) {
            for(auto x : g1.adjLists[v]) {
                addEdge((Vertex)v, (Vertex)x, g1origin);
            }
            for(auto x : g2.adjLists[v]) {
                addEdge((Vertex)v, (Vertex)x, g2origin);
            }
        }
    }

    void merge3Graphs(Graph& g1, const std::string& g1origin, Graph& g2, const std::string& g2origin, Graph& g3, const std::string& g3origin) {
        for(int v = 0; v < numVertices; v++) {
            for(auto x : g1.adjLists[v]) {
                addEdge((Vertex)v, (Vertex)x, g1origin);
            }
            for(auto x : g2.adjLists[v]) {
                addEdge((Vertex)v, (Vertex)x, g2origin);
            }
            for(auto x : g3.adjLists[v]) {
                addEdge((Vertex)v, (Vertex)x, g3origin);
            }
        }
    }

    void printGraph() {
        const char* VertexNames[] = {"AddProject", "DeleteProject", "WorksOn", "AddEmployee"};
        for(int v = 0; v < numVertices; v++) {
            printf("\n");
            std::cout << "Adjacency list of vertex " << VertexNames[v] << "\n head";
            for(auto x : adjLists[v]) {
                std::cout << "-> " << VertexNames[x] << " (from " << edgeOrigins[std::make_pair((Vertex)v, (Vertex)x)] << ")";
            }
            printf("\n");
        }
    }
void findCyclesUtil(int u, int color[], int pred[], int &cycleNumber) {
    color[u] = 1; // Mark current node as being visited (gray)

    for (auto v : adjLists[u]) {
        if (color[v] == 0) {
            // If neighbor v is not visited, continue DFS
            pred[v] = u;
            findCyclesUtil(v, color, pred, cycleNumber);
        } else if (color[v] == 1) {
            // Found a back edge to a gray node (cycle detected)
            cycleNumber++;

            // Trace back to find the cycle path
            std::vector<int> cycle;
            int cur = u;
            while (cur != v) {
                cycle.push_back(cur);
                cur = pred[cur];
            }
            cycle.push_back(v);
            cycle.push_back(u);

            // Print or store the cycle path as needed
            std::cout << "Cycle found:";
            for (int i = cycle.size() - 1; i >= 0; --i) {
                std::cout << " " << vertexNames[cycle[i]];
            }
            std::cout << std::endl;
        }
    }

    color[u] = 2; // Mark current node as completely visited (black)
}



int findCycles(int n) {
    int *color = new int[n];
    int *pred = new int[n];
    int cycleNumber = 0;

    // Initialize arrays
    for (int i = 0; i < n; ++i) {
        color[i] = 0; // White color (unvisited)
        pred[i] = -1; // No predecessor initially
    }

    // Perform DFS from each unvisited node
    for (int i = 0; i < n; ++i) {
        if (color[i] == 0) { // If node is unvisited
            findCyclesUtil(i, color, pred, cycleNumber);
        }
    }

    std::cout << "Total cycles found: " << cycleNumber << std::endl;

    delete[] color;
    delete[] pred;
    return cycleNumber;
}

void creategbox(){
        const char* VertexNames[] = {"AddProject", "DeleteProject", "WorksOn", "AddEmployee"};
        std::vector<std::vector<bool>> check_flag(numVertices, std::vector<bool>(numVertices));
        int cycleNumber1 = 0;
        int cycleNumber2 = 0;
        for(int v = 0; v < numVertices; v++) {
            //printf("\n");
            //std::cout << "Adjacency list of vertex " << VertexNames[v] << "\n head";
            for(auto x : adjLists[v]) {
                if(check_flag[v][x]==false){
                  if(edgeOrigins[std::make_pair((Vertex)v, (Vertex)x)]== "State Conflicts Graph"){
                    check_flag[v][x]=true;
                    check_flag[x][v]=true;
                    removeEdge((Vertex)v, (Vertex)x);
                    cycleNumber1= findCycles(numVertices);
                    addEdge((Vertex)v, (Vertex)x, "gbox");
                    removeEdge((Vertex)x, (Vertex)v);
                    cycleNumber2= findCycles(numVertices);
                    addEdge((Vertex)x, (Vertex)v, "gbox");
                    if(cycleNumber1>cycleNumber2)
                      removeEdge((Vertex)x, (Vertex)v);
                    else 
                      removeEdge((Vertex)v, (Vertex)x);
                  }
                }
            }
            printf("\n");
        }
}




};

int main() {
    Graph dependency(4);  // Creating a dependency graph with 4 vertices.
    Graph conflicts(4);  // Creating a conflicts graph with 4 vertices.
    Graph state_conflicts(4);  // Creating a state conflicts graph with 4 vertices.

    const char* VertexNames[] = {"AddProject", "DeleteProject", "WorksOn", "AddEmployee"};

    dependency.addEdge(DeleteProject, AddProject, "Dependency Graph");

    conflicts.addEdge(DeleteProject, WorksOn, "Conflicts Graph");
    conflicts.addEdge(WorksOn, DeleteProject, "Conflicts Graph");

    state_conflicts.addEdge(AddProject, WorksOn, "State Conflicts Graph");
    state_conflicts.addEdge(WorksOn, AddProject, "State Conflicts Graph");
    state_conflicts.addEdge(WorksOn, AddEmployee, "State Conflicts Graph");
    state_conflicts.addEdge(AddEmployee, WorksOn, "State Conflicts Graph");

    dependency.reverseEdges();  // Reverse all edges of the dependency graph.

    Graph cyclegraph(4);  // Creating the gbox graph.

    Graph gbox(4);
    std::vector<std::vector<int>> cycles;

    cyclegraph.merge3Graphs(conflicts, "Conflicts Graph", dependency, "Dependency Graph", state_conflicts, "State Conflicts Graph");  // Merge conflicts, dependency and state_conflicts into gbox.

    //cyclegraph.findCycles();
    std::cout << "--------------------------------------------------------------------" << std::endl;
    cyclegraph.findCycles(4);

    //cyclegraph.removeEdge(AddProject, DeleteProject);

    //cyclegraph.removeEdge(DeleteProject, WorksOn);
    //cyclegraph.removeEdge(WorksOn, DeleteProject);

    //cyclegraph.removeEdge(AddProject, WorksOn);
    //cyclegraph.removeEdge(WorksOn, AddProject);
    //cyclegraph.removeEdge(WorksOn, AddEmployee);
    //cyclegraph.removeEdge(AddEmployee, WorksOn);
    cyclegraph.findCycles(4);
    cyclegraph.printGraph();
    cyclegraph.creategbox();

    cyclegraph.printGraph();  // Print the gbox graph.

    gbox.merge2Graphs(conflicts, "Conflicts Graph", dependency, "Dependency Graph");

    return 0;
}
