#include "graph.h"
#include "logger.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include <stdexcept>
#include <iostream>

using json = nlohmann::json;

Graph load_graph(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open graph file: " + path);
    }

    json j;
    file >> j;

    Graph g;
    g.num_nodes = j["num_nodes"];
    g.adj.resize(g.num_nodes);

    for (const auto& e : j["edges"]) {
        Edge edge;
        edge.src    = e["src"];
        edge.dst    = e["dst"];
        edge.weight = e["weight"];

        // validate edge
        if (edge.weight <= 0) {
            throw std::runtime_error("Edge weight must be positive");
        }
        if (edge.src < 0 || edge.src >= g.num_nodes ||
            edge.dst < 0 || edge.dst >= g.num_nodes) {
            throw std::runtime_error("Edge node id out of range");
        }

        g.edges.push_back(edge);
        g.adj[edge.src].push_back({edge.dst, edge.weight});
    }

    LOG_INFO("[graph] Loaded " << g.num_nodes << " nodes and " << g.edges.size() << " edges from " << path);

    return g;
}