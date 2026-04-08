#pragma once
#include <vector>
#include <string>

// Represents a single directed edge with a positive weight
struct Edge {
    int src;
    int dst;
    int weight;
};

// Represents the full graph loaded from graph.json
struct Graph {
    int num_nodes;
    std::vector<Edge> edges;
    // adjacency list: adj[i] = list of (neighbor, weight)
    std::vector<std::vector<std::pair<int,int>>> adj;
};

// Load graph from a JSON file
Graph load_graph(const std::string& path);
