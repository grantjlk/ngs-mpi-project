# NGS-MPI Project Report

## Design Overview

This project generates random graphs using NetGameSim, partitions them across MPI ranks, and runs distributed leader election and Dijkstra shortest paths.

## Tools

### Graph Export (Scala)
Located in `NetGameSim/src/main/scala/Export.scala`. Loads the binary .ngs file using NetGameSim's built-in deserializer, remaps node IDs to 0..N-1, scales edge costs to integer weights in range 1-20, and outputs graph.json.

### Partitioner (Python)
Located in `tools/partition/partition.py`. Uses contiguous range partitioning — each node ID is assigned to a rank based on `rank = nodeId * numRanks / numNodes`. Produces part.json with an ownership map.

## Algorithm Choices

### Leader Election
FloodMax algorithm. Each node starts with its own ID as candidate, floods the max seen to all neighbors each round, converges after diameter rounds.

### Distributed Dijkstra
Parallel Dijkstra with global minimum reduction each iteration using MPI_Allreduce.

## Experiments
(To be filled in after runs)
