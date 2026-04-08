#pragma once
#include <vector>
#include <string>

// Stores which rank owns each node
struct Partition {
    int num_ranks;
    int num_nodes;
    // ownership[node_id] = rank that owns it
    std::vector<int> ownership;
};

// Load partition from a JSON file
Partition load_partition(const std::string& path);

// Get list of node ids owned by a given rank
std::vector<int> get_owned_nodes(const Partition& part, int rank);
