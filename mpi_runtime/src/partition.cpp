#include "partition.h"
#include "logger.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include <stdexcept>
#include <iostream>

using json = nlohmann::json;

Partition load_partition(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open partition file: " + path);
    }

    json j;
    file >> j;

    Partition p;
    p.num_ranks = j["num_ranks"];
    p.num_nodes = j["num_nodes"];
    p.ownership.assign(p.num_nodes, -1);

    for (const auto& [key, val] : j["ownership"].items()) {
        int node_id = std::stoi(key);
        int rank    = val;
        if (node_id < 0 || node_id >= p.num_nodes) {
            throw std::runtime_error("Node id out of range in partition file");
        }
        p.ownership[node_id] = rank;
    }
    // verify every node was assigned a valid rank
    for (int i = 0; i < p.num_nodes; i++) {
        if (p.ownership[i] == -1) {
            throw std::runtime_error("Node " + std::to_string(i) + " has no rank assignment in partition file");
        }
        if (p.ownership[i] < 0 || p.ownership[i] >= p.num_ranks) {
            throw std::runtime_error("Node " + std::to_string(i) + " assigned to invalid rank " + std::to_string(p.ownership[i]));
        }
    }

    LOG_INFO("[partition] Loaded " << p.num_nodes << " nodes across " << p.num_ranks << " ranks");

    return p;
}

std::vector<int> get_owned_nodes(const Partition& part, int rank) {
    std::vector<int> owned;
    for (int i = 0; i < part.num_nodes; i++) {
        if (part.ownership[i] == rank) {
            owned.push_back(i);
        }
    }
    return owned;
}