# Hamsou — Artifact for the Paper

This repository contains **Hamsou**, a C++ prototype that evaluates multiple replication protocols (e.g., Optimistic Replication, ECROS, CRDT message passing) across several applications/use-cases (e.g., Stack, Set, Movie, (Simple/Complex) Courseware, Project).

The workflow is always:

1) **Select** protocol + application in `tcp/config.hpp`  
2) **Generate** workload files using the matching benchmark generator in `wellcoordination/benchmark/`  
3) **Build** the runtime in `tcp/`  
4) **Run** `main-replication-protocols` on *N* nodes (or *N* processes) using the generated workload files

---

## Repository layout (important paths)

- `tcp/`  
  Main runtime, protocol selection, build scripts:
  - `tcp/config.hpp` (choose protocol + application)
  - `tcp/main-replication-protocols.cpp` (workload input directory)
  - `tcp/compile.sh` (build helper)

- `wellcoordination/benchmark/`  
  Benchmark/workload generators (C++). Each generator writes per-node workload files `1.txt .. N.txt`.

- `wellcoordination/workload/`  
  Default output location for generated workloads (you can change this to any absolute path).

- `run.sh`  
  Example launcher for **Slurm clusters** (uses ssh to start processes on allocated nodes).

- `OptimisticReplication.ipynb`  
  Example launcher for **Google Cloud** (collects IPs and starts processes).

---

## Prerequisites

- Linux machine(s) (local or cluster)
- `g++` (C++17), `cmake`, `make`
- `ssh` access between nodes (for `run.sh`)
- Optional (only if your build complains about Conan): `conan`

Example (Ubuntu):
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake python3 openssh-client
# Optional:
pip3 install --user conan
```

---

## Step 1 — Select protocol + application (`tcp/config.hpp`)

Open: `tcp/config.hpp`

### 1) Choose exactly ONE protocol
Uncomment one of these lines:
```cpp
// #define CRDT_MESSAGE_PASSING
// #define OPTIMISTIC_REPLICATION
// #define ECROS
```

Example: run Optimistic Replication
```cpp
#define OPTIMISTIC_REPLICATION
```

### 2) Choose exactly ONE application/use-case
Uncomment one of these lines:
```cpp
// #define STACK
// #define SET
// #define MOVIE
// #define COURSEWARE
// #define COMPLEXCOURSEWARE
// #define PROJECT
```

Example: run Courseware
```cpp
#define COURSEWARE
```

---

## Step 2 — Generate workload files (benchmark generator)

### 2.1 Pick the correct benchmark generator
Go to:
```bash
cd wellcoordination/benchmark
```

Each application has its own generator, for example:
- `complex-courseware-benchmark.cpp`
- `courseware-benchmark.cpp`
- `stack-benchmark.cpp`
- `set-benchmark.cpp`
- `movie-benchmark.cpp`
- `project-benchmark.cpp`
- `crdt-benchmark.cpp`

### 2.2 Update the output directory inside the benchmark code
At the top of each `*-benchmark.cpp`, there is a variable like:
```cpp
std::string loc = "/home/.../wellcoordination/workload/";
```
Change it to an absolute path on your machine, for example:
```cpp
std::string loc = "/home/<USER>/Hamsou/wellcoordination/workload/";
```

### 2.3 Compile and run the generator
Example for Complex Courseware:
```bash
g++ complex-courseware-benchmark.cpp -O2 -std=c++17 -o complex-courseware-benchmark.out
./complex-courseware-benchmark.out <nr_procs> <num_ops> <write_percentage>
```

Example values:
```bash
./complex-courseware-benchmark.out 4 100000 30
```

This creates per-node files under a directory like:
```
<loc>/<nr_procs>-<num_ops>-<write_percentage>/<usecase>/
  1.txt
  2.txt
  ...
  nr_procs.txt
```

> **Important:** Generate workloads once (on a single machine), then copy the produced workload directory to every node (same path on all nodes), because the runtime expects the same directory structure everywhere.

---

## Step 3 — Point the runtime to the same workload directory

Open:
- `tcp/main-replication-protocols.cpp`

Near the top you will see something like:
```cpp
std::string loc = "/home/.../wellcoordination/workload/";
```

Update it to match the `loc` you used in the benchmark generator.

---

## Step 4 — Build the runtime (`tcp/`)

```bash
cd tcp
source compile.sh
```

The binary is produced under:
```
tcp/build/main-replication-protocols
```

### If you get an error about `conanbuildinfo.cmake`
Some environments require running Conan once before `cmake`:

```bash
cd tcp
mkdir -p build
cd build
conan profile detect --force 2>/dev/null || true
conan install .. --build=missing
cd ..
source compile.sh
```

---

## Step 5 — Run on N nodes (or N processes)

Run the binary on **each node** with the same host list in the same order.

### Command-line arguments

From `tcp/build/`:
```bash
./main-replication-protocols \
  <id> <numnodes> <num_ops> <write_percent> <usecase> <operationrate> <failed_node_id> \
  <ip1> <ip2> ... <ipN>
```

- `<id>`: **1..N** (must match workload file `<id>.txt`)
- `<numnodes>`: N
- `<num_ops>`: number of operations used in workload generation
- `<write_percent>`: the same percentage used in workload generation (e.g., 30)
- `<usecase>`: directory name for the workload (e.g., `complex-courseware`, `courseware`, `stack`, `set`, `movie`, `project`, `crdt`)
- `<operationrate>`: operations-per-second knob used by the runtime (use a very large value to avoid throttling, e.g., `800000000`)
- `<failed_node_id>`: `0` for no injected failure (or set to a node id to inject failure when enabled)
- `<ip1>.. <ipN>`: IPs/hostnames of the N nodes in order

### Example (4 nodes)
Assume these IPs:
- node1: `10.0.0.1`
- node2: `10.0.0.2`
- node3: `10.0.0.3`
- node4: `10.0.0.4`

On node1:
```bash
cd tcp/build
./main-replication-protocols 1 4 100000 30 complex-courseware 800000000 0 10.0.0.1 10.0.0.2 10.0.0.3 10.0.0.4
```

On node2:
```bash
cd tcp/build
./main-replication-protocols 2 4 100000 30 complex-courseware 800000000 0 10.0.0.1 10.0.0.2 10.0.0.3 10.0.0.4
```

…and similarly for nodes 3 and 4 (using ids 3 and 4).

---

## Running with scripts

### A) Slurm clusters (`run.sh`)
`run.sh` is a Slurm job script that:
1) Compiles a selected benchmark generator
2) Generates workload files
3) Starts the runtime on the allocated nodes using `ssh`

What you typically need to edit in `run.sh`:
- `OPREP_HOME` (path to the repo on each node)
- `RESULT_LOC` (where logs/results should be written)
- `USECASE`, `NUM_NODES`, `NUM_OPS`, `WRITE_PERC`, `OperationRate`

Submit:
```bash
sbatch run.sh
```

### B) Google Cloud (`OptimisticReplication.ipynb`)
The notebook `OptimisticReplication.ipynb` contains a more updated workflow for Google Cloud:
- Create instances / gather IPs
- Sync code/workloads to instances
- Run `./main-replication-protocols ...` on each instance

---

## Troubleshooting

- **Workload files not found**
  - Ensure the `loc` path in:
    - benchmark generator (`wellcoordination/benchmark/*-benchmark.cpp`)
    - runtime (`tcp/main-replication-protocols.cpp`)
    are identical and exist on every node.

- **Wrong node id / wrong workload**
  - `<id>` must be 1..N and matches the workload file `<id>.txt`.

- **Nodes cannot connect**
  - Verify IP list order is identical on all nodes.
  - Ensure required ports are open in firewall/security-group settings.

- **Build errors**
  - Make sure you have a modern compiler (C++17).
  - If the error mentions Conan, run the Conan commands shown above.

---

## Citation
If you use this artifact in academic work, please cite the associated paper (see the paper/ artifact documentation in the repository or the project website).

