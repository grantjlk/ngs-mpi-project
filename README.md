Sure! Here it is with no em dashes:
markdown# NGS-MPI: Distributed Graph Algorithms with MPI

End-to-end distributed computing pipeline that generates random graphs using
NetGameSim, partitions them across MPI ranks, and runs distributed leader election
and Dijkstra shortest paths using C++17 and OpenMPI.

## Pipeline
NetGameSim (Scala) -> Export.scala -> graph.json
|
partition.py -> part.json
|
C++17 MPI runtime (ngs_mpi)
- Leader Election (FloodMax)
- Dijkstra Shortest Paths
-> metrics output

## Language Stack

- Graph generation: Scala/SBT (NetGameSim)
- Graph export: Scala (Export.scala)
- Graph partitioning: Python 3
- Core MPI runtime and algorithms: C++17 + OpenMPI

The core distributed algorithms (leader election and Dijkstra) are implemented
in C++17 with OpenMPI. This is the required language stack for the MPI runtime.

## Prerequisites

Install the following on Ubuntu or WSL2:

```bash
# Java (required for NetGameSim and Export.scala)
sudo apt install openjdk-17-jdk

# SBT (Scala build tool, required to build NetGameSim)
echo "deb https://repo.scala-sbt.org/scalasbt/debian all main" | \
  sudo tee /etc/apt/sources.list.d/sbt.list
curl -sL "https://keyserver.ubuntu.com/pks/lookup?op=get&search=0x2EE0EA64E40A89B84B2DF73499E82A75642AC823" | \
  sudo apt-key add
sudo apt update && sudo apt install sbt

# Python 3
sudo apt install python3

# OpenMPI (required for the MPI runtime -- do not use MPICH on WSL2)
sudo apt install libopenmpi-dev openmpi-bin

# CMake and C++ compiler
sudo apt install cmake g++

# Google Test (required for unit and MPI tests)
sudo apt install libgtest-dev
```

## Repository Structure
NetGameSim/                        Upstream NetGameSim repo (lives outside this repo,
see Setup below)
tools/
graph_export/
export.sh                      Shell script to invoke Export.scala
Export.scala                   Custom graph exporter (copy into NetGameSim to use)
partition/
partition.py                   Graph partitioner (contiguous or random strategy)
mpi_runtime/
src/                             C++17 MPI runtime and algorithm implementations
include/                         Shared headers
CMakeLists.txt                   CMake build configuration
configs/                           Example NetGameSim configuration files
experiments/
run_experiments.sh               Script to reproduce all experiment runs
outputs/
graph.json                       Pre-generated 501-node graph (committed)
REPORT.md                          Experiment writeup
README.md                          This file

## Setup

This project expects NetGameSim to be cloned as a sibling directory next to this repo:
453project/
NetGameSim/       <- clone from upstream
my-project/       <- this repo

Clone NetGameSim:

```bash
cd ~/453project
git clone <netgamesim-repo-url> NetGameSim
```

## Build

```bash
cd ~/453project/my-project

cmake -S mpi_runtime -B build
cmake --build build
```

This produces three binaries in `build/`:
- `ngs_mpi` -- main MPI runtime
- `unit_tests` -- unit tests (no MPI required)
- `mpi_tests` -- MPI integration tests

## Running the Pipeline

A pre-generated `outputs/graph.json` is included in the repository. You can skip
to the Partition step if you do not want to set up NetGameSim.

### Step 1: Generate a Graph (optional)

```bash
cd ~/453project/NetGameSim
sbt clean assembly

java -Xms2G -Xmx8G -jar target/scala-3.2.2/netmodelsim.jar TestGraph
cd ..
```

Note: NetGameSim uses ThreadLocalRandom internally and is not fully reproducible
across runs even with the same seed. Use the committed graph.json for exact
reproducibility.

### Step 2: Export to JSON (optional)

First copy Export.scala into NetGameSim and rebuild:

```bash
cp tools/graph_export/Export.scala \
  ~/453project/NetGameSim/src/main/scala/Export.scala

cd ~/453project/NetGameSim
sbt clean assembly
cd ~/453project/my-project
```

Then run the exporter:

```bash
./tools/graph_export/export.sh \
  ~/453project/NetGameSim/TestGraph.ngs \
  outputs/graph.json
```

### Step 3: Partition the Graph

```bash
# Contiguous partitioning
python3 tools/partition/partition.py outputs/graph.json \
  --ranks 4 --out outputs/part.json --strategy contiguous

# Random partitioning
python3 tools/partition/partition.py outputs/graph.json \
  --ranks 4 --out outputs/part.json --strategy random
```

### Step 4: Run the Algorithms

```bash
# Leader election
mpirun -n 4 ./build/ngs_mpi \
  --graph outputs/graph.json \
  --part outputs/part.json \
  --algo leader --rounds 200

# Dijkstra shortest paths from source node 0
mpirun -n 4 ./build/ngs_mpi \
  --graph outputs/graph.json \
  --part outputs/part.json \
  --algo dijkstra --source 0
```

To run with more ranks than physical cores:

```bash
mpirun --oversubscribe -n 8 ./build/ngs_mpi \
  --graph outputs/graph.json \
  --part outputs/part.json \
  --algo leader --rounds 200
```

## Running Tests

Tests must be run from the `mpi_runtime/` directory:

```bash
cd mpi_runtime

# Unit tests (no MPI required)
../build/unit_tests

# MPI integration tests
mpirun -n 4 ../build/mpi_tests
```

All 11 tests should pass (7 unit, 4 MPI).

## Reproducing Experiments

```bash
./experiments/run_experiments.sh
```

Results are saved to `experiments/results.txt`. The script runs:
- Experiment 1: contiguous vs random partitioning at 4 ranks
- Experiment 2: scalability at 1, 2, 4, and 8 ranks using contiguous partitioning

## Notes

- `outputs/graph.json` is committed so the grader can skip NetGameSim setup
- Use OpenMPI, not MPICH. MPICH 4.2.0 returns size=1 for all ranks on WSL2
- All algorithms assume an undirected graph. The exporter adds reverse edges to
  guarantee full connectivity since NetGameSim directed graphs are not strongly
  connected from all sources
- The 8-rank experiment requires --oversubscribe on machines with fewer than 8 cores
Once you paste this into README.md, commit everything together:
bashcd ~/453project/my-project
git add README.md REPORT.md tools/graph_export/Export.scala
git commit -m "Add README, finalize REPORT, add Export.scala"
git push
Then just add the TA and submit. How does it look?