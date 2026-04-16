# NGS-MPI: Distributed Graph Analytics Report

## 1. System Overview

This project implements a complete end-to-end distributed computing pipeline for
graph analysis. The goal is to take a randomly generated graph, partition it across
multiple MPI processes, and run two classic distributed algorithms on it: leader
election and single-source shortest paths.

* **Generation:** Scala (NetGameSim) generates high-level binary `.ngs` graph models.
* **Ingestion:** `Export.scala` transforms binary data into a portable, weighted JSON format.
* **Partitioning:** A Python-based tool maps nodes to MPI ranks using **Contiguous**
  or **Random** strategies and emits an ownership map.
* **Analytics:** A C++17 OpenMPI runtime loads the graph and partition, then executes
  FloodMax Leader Election and Parallel Dijkstra with global reductions.

---

## 2. Approach and Overall Idea

The key design decision in this project is that MPI ranks are not graph nodes.
Instead, each rank owns a *subset* of nodes, which is how real distributed systems
work when the number of logical entities exceeds the number of processes. This
separation allows the system to scale the graph size independently of the number
of MPI processes.

Rather than implementing point-to-point message passing between individual nodes,
we use `MPI_Allreduce` collectives as the communication primitive. This trades
fine-grained messaging for simpler, deadlock-free synchronization. Every rank
participates in every collective in the same order, which guarantees correctness.

All ranks load the full graph and partition at startup. This replicate-everything
memory model eliminates the need for ghost node bookkeeping and remote edge
relaxation messages, at the cost of memory scaling. For graphs of this size
(hundreds to low thousands of nodes) this is a practical tradeoff.

---

## 3. Technical Design

### Data Engineering

* **Graph Normalization:** Node IDs are remapped to $0 \dots N-1$. Weights are
  scaled to integers using $\max(1, \lfloor\text{cost} \times 20\rfloor)$, where
  cost is NetGameSim's internal [0,1] action cost. This guarantees positive integer
  weights as required by Dijkstra.
* **Connectivity Assurance:** NetGameSim  generates directed graphs. To ensure correctness for both algorithms,
  the exporter mirrors every directed edge, producing an undirected graph. This
  is to guarantee 100% reachability from any source node..
* **Partitioning Logic:**
    * **Contiguous:** Assigns nodes via $\lfloor\text{nodeId} \times
      \text{numRanks} / \text{numNodes}\rfloor$, producing contiguous ranges per rank.
    * **Random:** Shuffles node IDs with a fixed seed before assignment, breaking
      spatial locality to test sensitivity to partition quality.

### Distributed Algorithms

* **Leader Election (FloodMax):** Each node starts with its own ID as its candidate.
  In each synchronous round, every rank updates its owned nodes by propagating the
  maximum candidate seen across all neighbors. A single `MPI_Allreduce` with
  `MPI_MAX` over the full candidate vector synchronizes all ranks at the end of
  each round. A second `MPI_Allreduce` detects global convergence and triggers
  early termination when no candidate changed. The elected leader is the maximum
  node ID in the graph, which all ranks agree on after convergence.

* **Distributed Dijkstra (Replicated Graph Model):** Uses a two-phase synchronization per iteration:
    1. Each rank finds its locally best unsettled node and proposes it via
       `MPI_Allreduce` (MIN) to determine the globally minimum tentative distance.
    2. A second `MPI_Allreduce` (MAX) over node IDs resolves which specific node
       holds that distance, breaking ties deterministically by choosing the largest ID.
    3. All ranks settle that node and relax its outgoing edges locally, since every
       rank holds the full adjacency list.

* **Memory Model:** All ranks load the full adjacency list at startup. This
  simplifies edge relaxation by removing ghost node messaging overhead, at the cost
  of O(N+E) memory per rank. For graphs in the hundreds-of-nodes range this is
  acceptable. A production system would partition the adjacency list and use
  point-to-point messages for cross-rank relaxations.

### Correctness Assumptions

The following assumptions are required for correct results:

* All edge weights must be positive (guaranteed by the exporter's weight formula).
* The graph must be connected (guaranteed by edge mirroring in the exporter).
* Node IDs must be in the range 0 to N-1 (enforced by the exporter's ID remapping).
* The partition file must assign exactly one rank to every node, and the number of
  ranks in the partition must match the MPI launch size (enforced at runtime).

---

## 4. Implementation Details

### Graph Export (Export.scala)

The exporter loads the NetGameSim binary `.ngs` file, remaps node IDs to a
contiguous 0-based range, extracts edge weights from the `Action.cost` field,
scales them to positive integers, and mirrors every edge to produce an undirected
graph. The output is a JSON file with `num_nodes`, a `nodes` array, and an `edges`
array with `src`, `dst`, and `weight` fields.

One important detail: NetGameSim's `fromId`/`toId` fields in edge objects are
random numbers unrelated to node identity. The exporter uses `fromNode.id` and
`toNode.id` instead, which are the correct identifiers.

### Partitioner (partition.py)

The partitioner reads the graph JSON, assigns each node to a rank according to the
chosen strategy, and writes an ownership map as a JSON object keyed by node ID
string. The random strategy uses a fixed seed (default 42) for reproducibility.
Edge cut statistics are printed to aid experiment analysis.

### MPI Runtime (C++17 / OpenMPI)

The runtime is structured as separate compilation units: `graph.cpp` and
`partition.cpp` handle loading and validation, `leader_election.cpp` and
`dijkstra.cpp` implement the algorithms, and `metrics.cpp` handles timing and
output. Each algorithm returns results to `main.cpp` which handles printing.

Input validation is performed before any algorithm runs: the partition rank count
must match the MPI size, the graph and partition node counts must agree, and the
source node must be within range.

---

## 5. Experimental Results

**Environment:** 501 nodes, 1984 edges, WSL2 (Ubuntu 24.04), OpenMPI,
`--oversubscribe` for runs exceeding physical core count.

### Experiment 1: Partitioning Strategy Comparison (4 Ranks)

**Hypothesis:** Random partitioning will produce more edge cuts than contiguous
partitioning because it ignores node ID locality. More edge cuts mean more
cross-rank communication, which should increase runtime for both algorithms.

**Expected Results:** Random partitioning will have higher edge cuts and slower
runtimes than contiguous partitioning.

| Strategy | Edge Cuts | LE Rounds | LE Runtime | Dijkstra Runtime |
| :--- | :--- | :--- | :--- | :--- |
| **Contiguous** | 75.3% | 8 | 0.0018s | 0.0111s |
| **Random** | 75.4% | 10 | 0.0014s | 0.0128s |

**Actual Results and Analysis:** The edge cut results were basically identical
(75.3% vs 75.4%), which was not what I expected. I originally thought random
partitioning would produce significantly more cross-rank edges, but that did
not happen here. This suggests that NetGameSim graphs do not have meaningful
locality with respect to node IDs. The IDs are effectively arbitrary, so
contiguous partitioning does not preserve any useful structure.

Because of this, the runtime differences are very small.
Random partitioning was slightly faster for leader election (0.0014s vs 0.0018s),
but also required more rounds (10 vs 8). Given how small the timing differences
are, this is likely measurement noise rather than a meaningful performance effect.

Dijkstra shows a small slowdown under random partitioning (0.0128s vs 0.0111s),
which is consistent with the hypothesis, but the difference is minor. Overall,
these results indicate that partitioning strategy has little impact on performance
for this graph, primarily because the edge cut rate is already very high in both
cases (~75%).

**Key Insight:** On this graph, partition quality has minimal impact on performance because node IDs do not encode meaningful locality. As a result, both contiguous and random partitioning produce similarly high edge cut rates, so the system’s performance is dominated by algorithmic communication patterns rather than how the graph is split across ranks.

---

### Experiment 2: Scalability with Varying Rank Count (Contiguous)

**Hypothesis:** Increasing ranks will speed up Dijkstra up to a point, after which
collective overhead will dominate and performance will degrade. Leader election
should converge in fewer rounds at lower rank counts due to shorter logical
distances within each partition.

**Expected Results:** Peak speedup somewhere between 2 and 4 ranks, with
degradation at 8 ranks due to Allreduce overhead scaling with rank count.

| Ranks | LE Rounds | LE Runtime | Dijkstra Runtime | Speedup ($T_1/T_n$) |
| :--- | :--- | :--- | :--- | :--- |
| 1 | 5 | 0.0006s | 0.0161s | 1.00x |
| 2 | 8 | 0.0007s | 0.0086s | 1.87x |
| 4 | 8 | 0.0011s | 0.0126s | 1.28x |
| 8 | 8 | 0.0009s | 0.0185s | 0.87x |

**Actual Results and Analysis:** Peak speedup occurred at 2 ranks (1.87x), which
matches the hypothesis. At 4 ranks performance dropped to 1.28x, and at 8 ranks
the system became slower than the single-rank baseline (0.87x).

The main reason for this is the cost of collective communication. Dijkstra performs
two `MPI_Allreduce` operations per iteration, and with 501 iterations this creates
a large amount of synchronization overhead. As the number of ranks increases, the
latency cost of these collectives grows and begins to dominate the runtime.

This effect is especially visible beyond 2 ranks, where additional processes increase synchronization overhead without reducing the number of iterations required for convergence. Each iteration still requires the same fixed number of global reductions, so scalability is limited by communication rather than computation.

Leader election rounds remained stable at 8 from 2 to 8 ranks, suggesting that the
logical propagation distance in the graph did not change significantly with further
partitioning. The single-rank case converged in 5 rounds because there is no
cross-rank synchronization, so propagation follows the graph diameter directly.

**Key Insight:** The scalability results show that the system is fundamentally communication-bound rather than compute-bound. Because each iteration of Dijkstra requires a fixed number of global MPI_Allreduce operations, increasing the number of ranks increases synchronization cost without reducing the number of iterations, leading to diminishing and eventually negative returns from parallelism.

---

## 6. Limitations and Future Work

* **Message Metrics:** Counts reflect logical `Allreduce` calls rather than actual
  network packets. OpenMPI implements collectives using tree-based or recursive
  doubling algorithms internally, so actual message counts are higher. The reported
  numbers are best interpreted as a lower bound on communication volume.
* **Memory Scaling:** Replicating the full graph on every rank limits scalability
  to larger graphs. A production implementation would partition the adjacency list
  and use point-to-point `MPI_Send`/`MPI_Recv` for cross-rank edge relaxations.
* **Partitioning:** Both strategies produce high edge cut rates (~75%) on
  NetGameSim graphs due to their lack of spatial locality. METIS-style recursive
  bisection could reduce edge cuts on graphs with natural clustering structure.
* **Sparse Reductions:** Leader election currently broadcasts the full candidate
  vector each round via Allreduce. Propagating only changed values would reduce
  bandwidth significantly for large sparse graphs.
  