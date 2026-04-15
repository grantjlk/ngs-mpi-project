# NGS-MPI: Distributed Graph Analytics Report

## 1. System Overview
This project implements a complete distributed computing pipeline for graph analysis.
* **Generation:** Scala (NetGameSim) generates high-level binary `.ngs` graph models.
* **Ingestion:** `Export.scala` transforms binary data into a portable, weighted JSON format.
* **Partitioning:** A Python-based tool maps nodes to MPI ranks using **Contiguous** or **Random** strategies.
* **Analytics:** A C++17 OpenMPI runtime executes FloodMax Leader Election and Parallel Dijkstra.

---

## 2. Technical Design

### Data Engineering
* **Graph Normalization:** Node IDs are remapped to $0 \dots N-1$. Weights are scaled to integers using $\max(1, (\text{cost} \times 20))$.
* **Connectivity Assurance:** To ensure correctness for Dijkstra, the graph is treated as undirected (edges are mirrored). This guarantees 100% reachability from any source node.
* **Partitioning Logic:**
    * **Contiguous:** Assigns nodes via $\text{nodeId} \times \text{numRanks} / \text{numNodes}$.
    * **Random:** Shuffles node IDs before assignment to test the system's sensitivity to spatial locality.

### Distributed Algorithms
* **Leader Election (FloodMax):** Nodes propagate candidate IDs across rounds. We utilize `MPI_Allreduce` with `MPI_MAX` for global synchronization and an early-exit check to terminate once candidates stabilize.
* **Distributed Dijkstra:** Uses a two-step synchronization per iteration:
    1. `MPI_Allreduce` (MIN) to determine the global minimum distance among unsettled nodes.
    2. `MPI_Allreduce` (MAX) to identify the specific node ID that owns that distance.
* **Memory Model:** All ranks load the full adjacency list. This simplifies edge relaxation by removing "ghost node" messaging overhead, prioritizing performance over memory-scaling at this graph size.

---

## 3. Experimental Results
**Environment:** 501 nodes, 1984 edges, WSL2 (Ubuntu 24.04), OpenMPI.

### Experiment 1: Partitioning Strategy (4 Ranks)
| Strategy | Edge Cuts | LE Runtime | Dijkstra Runtime |
| :--- | :--- | :--- | :--- |
| **Contiguous** | 75.3% | 0.0005s | 0.0058s |
| **Random** | 75.4% | 0.0013s | 0.0070s |

**Analysis:** NetGameSim graphs lack strong spatial locality by default. While edge cuts remained nearly identical, the **Random** strategy significantly increased Leader Election runtime ($2.6\times$ slower). This suggests that scrambling IDs increases the logical distance messages must travel before the maximum value propagates to all ranks.

### Experiment 2: Scalability (Contiguous Strategy)
| Ranks | LE Rounds | Dijkstra Runtime | Speedup ($T_1/T_n$) |
| :--- | :--- | :--- | :--- |
| 1 | 5 | 0.0085s | 1.00x |
| 2 | 501 | 0.0055s | **1.54x** |
| 4 | 501 | 0.0058s | 1.46x |
| 8 | 501 | 0.0088s | 0.96x |

**Analysis:** We observed a peak speedup at **2 ranks**. Beyond this point, the **communication overhead** of the 1,002 `MPI_Allreduce` calls required for Dijkstra began to outweigh the computational benefits. The $0.96\times$ speedup at 8 ranks illustrates a classic parallelization bottleneck where the system is "communication-bound."

---

## 4. Limitations & Future Work
* **Message Metrics:** Counts reflect logical `Allreduce` operations; actual network packets are managed by OpenMPI's tree-based collectives.
* **Optimization:** Future iterations could implement **Sparse Reductions** (only propagating changed values) to reduce bandwidth for Leader Election.
* **Partitioning:** Exploring **METIS-style recursive bisection** could further minimize edge cuts if applied to graphs with natural clustering.