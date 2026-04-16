# NGS-MPI: Distributed Graph Algorithms with MPI

End-to-end distributed computing pipeline that generates random graphs using NetGameSim, partitions them across MPI ranks, and runs distributed leader election and Dijkstra shortest paths using C++17 and OpenMPI.

---
## Quickstart

Run the project end-to-end using the included graph:

```bash
# build
cmake -S mpi_runtime -B build
cmake --build build

# partition graph
python3 tools/partition/partition.py outputs/graph.json \
  --ranks 4 --out outputs/part.json --strategy contiguous

# run leader election
mpirun -n 4 ./build/ngs_mpi \
  --graph outputs/graph.json \
  --part outputs/part.json \
  --algo leader --rounds 200

# run dijkstra
mpirun -n 4 ./build/ngs_mpi \
  --graph outputs/graph.json \
  --part outputs/part.json \
  --algo dijkstra --source 0
```
---

## Reproducibility

### Graph Generation

NetGameSim uses `ThreadLocalRandom`, which cannot be externally seeded.  
As a result, graph generation is not fully reproducible, even if a seed is specified in configuration files.

### Canonical Dataset

To ensure consistent grading and reproducibility:

- The repository includes a pre-generated graph:  
  `outputs/graph.json`
- This file is treated as the **canonical dataset**
- All experiments reported in `REPORT.md` were run using this graph

Graders should use this file to reproduce results without relying on NetGameSim.

### Partitioning

Graph partitioning **is reproducible**:

- The partitioning tool supports a `--seed` flag
- Default seed is `42`
- Given the same input graph and seed, partition outputs are deterministic

---
## Pipeline

```
NetGameSim (Scala)
    ↓
Export.scala → graph.json
    ↓
partition.py → part.json
    ↓
C++17 MPI runtime (ngs_mpi)
    - Leader Election (FloodMax)
    - Dijkstra Shortest Paths
    ↓
Metrics output
```

---

## Language Stack

* Graph generation: Scala / SBT (NetGameSim)
* Graph export: Scala (Export.scala)
* Graph partitioning: Python 3
* Core MPI runtime and algorithms: C++17 + OpenMPI

The distributed algorithms (leader election and Dijkstra) are implemented in C++17 using OpenMPI.

---

## Prerequisites

Install the following on Ubuntu or WSL2:

```bash
# Java (NetGameSim + exporter)
sudo apt install openjdk-17-jdk

# SBT
echo "deb https://repo.scala-sbt.org/scalasbt/debian all main" | \
  sudo tee /etc/apt/sources.list.d/sbt.list
curl -sL "https://keyserver.ubuntu.com/pks/lookup?op=get&search=0x2EE0EA64E40A89B84B2DF73499E82A75642AC823" | \
  sudo apt-key add
sudo apt update && sudo apt install sbt

# Python
sudo apt install python3

# OpenMPI (required — do not use MPICH on WSL2)
sudo apt install libopenmpi-dev openmpi-bin

# Build tools
sudo apt install cmake g++

# Testing
sudo apt install libgtest-dev
```

---

## Repository Structure

```
NetGameSim/                 (external, cloned separately)
tools/
  graph_export/
    export.sh
    Export.scala
  partition/
    partition.py
mpi_runtime/
  src/
  include/
  CMakeLists.txt
configs/
experiments/
  run_experiments.sh
outputs/
  graph.json
REPORT.md
README.md
```

---

## Setup

This project expects NetGameSim to be a sibling directory:

```
453project/
  NetGameSim/
  my-project/
```

Clone NetGameSim:

```bash
cd ~/453project
git clone <netgamesim-repo-url> NetGameSim
```

---

## Build

```bash
cd ~/453project/my-project

cmake -S mpi_runtime -B build
cmake --build build
```

Binaries produced in `build/`:

* `ngs_mpi` — main runtime
* `unit_tests` — unit tests
* `mpi_tests` — MPI tests

---

## Running the Pipeline

A pre-generated `outputs/graph.json` is included, so NetGameSim setup can be skipped.

### Step 1: Generate Graph (optional)

```bash
cd ~/453project/NetGameSim
sbt clean assembly

java -Xms2G -Xmx8G -jar target/scala-3.2.2/netmodelsim.jar TestGraph
cd ..
```


---

### Step 2: Export to JSON (optional)

```bash
cp tools/graph_export/Export.scala \
  ~/453project/NetGameSim/src/main/scala/Export.scala

cd ~/453project/NetGameSim
sbt clean assembly
cd ~/453project/my-project
```

```bash
./tools/graph_export/export.sh \
  ~/453project/NetGameSim/TestGraph.ngs \
  outputs/graph.json
```

---

### Step 3: Partition the Graph

```bash
# Contiguous
python3 tools/partition/partition.py outputs/graph.json \
  --ranks 4 --out outputs/part.json --strategy contiguous

# Random
python3 tools/partition/partition.py outputs/graph.json \
  --ranks 4 --out outputs/part.json --strategy random
```

---

### Step 4: Run Algorithms

```bash
# Leader election
mpirun -n 4 ./build/ngs_mpi \
  --graph outputs/graph.json \
  --part outputs/part.json \
  --algo leader --rounds 200

# Dijkstra
mpirun -n 4 ./build/ngs_mpi \
  --graph outputs/graph.json \
  --part outputs/part.json \
  --algo dijkstra --source 0
```

Oversubscribe if needed:

```bash
mpirun --oversubscribe -n 8 ./build/ngs_mpi ...
```

---

## Distributed Execution Model

* Each MPI rank loads the full graph
* Each rank owns a subset of nodes defined by the partition
* Execution follows a bulk-synchronous model using MPI collectives

### Leader Election (FloodMax)

* Each node maintains a candidate ID
* Each round:

  * Local updates propagate values
  * `MPI_Allreduce (MAX)` synchronizes global state
* Terminates when no changes occur globally

### Dijkstra

* Each iteration:

  * Each rank computes its local minimum tentative distance
  * Global minimum selected via `MPI_Allreduce`
  * All ranks relax edges from that node
* Continues until all nodes are settled

---

## Input Validation

The runtime validates all inputs before execution:

* Graph file must be valid JSON with consistent node count
* Partition file must:

  * Assign every node exactly once
  * Use valid ranks in `[0, num_ranks)`
  * Match MPI world size
* CLI arguments:

  * `--algo` must be `leader` or `dijkstra`
  * `--rounds` must be positive
  * `--source` must be within node range

Invalid inputs produce descriptive errors and exit cleanly.

---

## Metrics

The runtime reports:

* Runtime (seconds)
* Iterations
* Allreduce calls (logical communication steps)
* Approximate bytes sent

Note:
MPI collectives do not correspond directly to point-to-point messages.
We approximate communication cost as **O(ranks)** per collective for comparison purposes.

---

## Running Tests

```bash
# Unit tests — must be run from the mpi_runtime/ directory
# so that relative paths like "tests/small_graph.json" resolve correctly
cd mpi_runtime
../build/unit_tests

# MPI tests — also run from mpi_runtime/
mpirun -n 4 ../build/mpi_tests
```

All 11 tests should pass (7 unit, 4 MPI).


---

## Reproducing Experiments

```bash
./experiments/run_experiments.sh
```

Outputs saved to `experiments/results.txt`.

Experiments include:

* Contiguous vs random partitioning (4 ranks)
* Scalability across 1, 2, 4, 8 ranks

---

## Assumptions and Limitations

* Graphs are treated as undirected (exporter adds reverse edges)
* Graph is assumed to be connected
* Dijkstra requires non-negative edge weights
* Full graph is replicated on all ranks (not memory optimized)
* Communication uses synchronous collectives (no async overlap)

---

## Notes

* `outputs/graph.json` is included for reproducibility
* Use OpenMPI, MPICH may fail on WSL2
* Leader election correctness depends on graph connectivity
