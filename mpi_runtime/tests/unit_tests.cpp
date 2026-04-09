#include <gtest/gtest.h>
#include <fstream>
#include "graph.h"
#include "partition.h"

// ─── Graph Tests ───────────────────────────────────────────────────────────

TEST(GraphTest, LoadsCorrectly) {
    Graph g = load_graph("tests/small_graph.json");
    EXPECT_EQ(g.num_nodes, 5);
    EXPECT_EQ(g.edges.size(), 10);
}

TEST(GraphTest, AdjacencyListCorrect) {
    Graph g = load_graph("tests/small_graph.json");
    // node 0 should have edges to 1 and 3
    EXPECT_EQ(g.adj[0].size(), 2);
}

TEST(GraphTest, AllWeightsPositive) {
    Graph g = load_graph("tests/small_graph.json");
    for (const auto& e : g.edges) {
        EXPECT_GT(e.weight, 0) << "Edge " << e.src 
                               << "->" << e.dst 
                               << " has non-positive weight";
    }
}

TEST(GraphTest, NodeIdsInRange) {
    Graph g = load_graph("tests/small_graph.json");
    for (const auto& e : g.edges) {
        EXPECT_GE(e.src, 0);
        EXPECT_LT(e.src, g.num_nodes);
        EXPECT_GE(e.dst, 0);
        EXPECT_LT(e.dst, g.num_nodes);
    }
}

// ─── Partition Tests ────────────────────────────────────────────────────────

TEST(PartitionTest, LoadsCorrectly) {
    Partition p = load_partition("tests/small_part.json");
    EXPECT_EQ(p.num_ranks, 4);
    EXPECT_EQ(p.num_nodes, 5);
}

TEST(PartitionTest, AllNodesAssigned) {
    Partition p = load_partition("tests/small_part.json");
    EXPECT_EQ((int)p.ownership.size(), p.num_nodes);
    for (int i = 0; i < p.num_nodes; i++) {
        EXPECT_GE(p.ownership[i], 0);
        EXPECT_LT(p.ownership[i], p.num_ranks) 
            << "Node " << i << " assigned to invalid rank";
    }
}

TEST(PartitionTest, OwnedNodesCorrect) {
    Partition p = load_partition("tests/small_part.json");
    // rank 0 owns nodes 0 and 4
    std::vector<int> owned = get_owned_nodes(p, 0);
    EXPECT_EQ(owned.size(), 2);
}

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}