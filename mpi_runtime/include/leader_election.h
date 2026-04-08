#pragma once
#include "graph.h"
#include "partition.h"
#include "metrics.h"

// Run FloodMax leader election
// Returns the elected leader id (same on all ranks)
int run_leader_election(
    const Graph& g,
    const Partition& p,
    int rank,
    int num_ranks,
    int rounds,
    Metrics& metrics
);