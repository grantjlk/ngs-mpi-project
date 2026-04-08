#pragma once
#include "graph.h"
#include "partition.h"
#include "metrics.h"
#include <vector>

// Run distributed Dijkstra from source node
// Returns distance from source to every node
std::vector<int> run_dijkstra(
    const Graph& g,
    const Partition& p,
    int source,
    int rank,
    int num_ranks,
    Metrics& metrics
);