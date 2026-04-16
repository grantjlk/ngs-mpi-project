#include "dijkstra.h"
#include "logger.h"
#include <mpi.h>
#include <vector>
#include <iostream>
#include <chrono>
#include <algorithm>
#include <climits>

static const int INF = INT_MAX / 2;

std::vector<int> run_dijkstra(
    const Graph& g,
    const Partition& p,
    int source,
    int rank,
    int num_ranks,
    Metrics& metrics)
{
    auto start = std::chrono::high_resolution_clock::now();

    int num_nodes = g.num_nodes;

    std::vector<int> dist(num_nodes, INF);
    std::vector<bool> settled(num_nodes, false);
    dist[source] = 0;

    metrics.messages_sent = 0;
    metrics.bytes_sent    = 0;
    metrics.iterations    = 0;

    for (int iter = 0; iter < num_nodes; iter++) {
        metrics.iterations++;

        // each rank finds its local best unsettled node
        int local_best_dist = INF;
        int local_best_node = -1;

        for (int i = 0; i < num_nodes; i++) {
            if (p.ownership[i] == rank && !settled[i] && dist[i] < local_best_dist) {
                local_best_dist = dist[i];
                local_best_node = i;
            }
        }

        // find global minimum distance across all ranks
        int global_best_dist = INF;
        //checked mpi all reduce call
        int rc1 = MPI_Allreduce(
            &local_best_dist,
            &global_best_dist,
            1,
            MPI_INT,
            MPI_MIN,
            MPI_COMM_WORLD
        );
        if (rc1 != MPI_SUCCESS) MPI_Abort(MPI_COMM_WORLD, rc1);

        metrics.messages_sent += num_ranks;
        metrics.bytes_sent    += num_ranks * sizeof(int);

        // no more reachable unsettled nodes
        if (global_best_dist == INF) break;

        // find which node has that distance — broadcast from owning rank
        int global_best_node = -1;
        if (local_best_dist == global_best_dist) {
            global_best_node = local_best_node;
        }

        // use max reduction to find the node id
        int reduced_best_node = -1;
        // use checked mpi call
        int rc2 = MPI_Allreduce(
            &global_best_node,
            &reduced_best_node,
            1,
            MPI_INT,
            MPI_MAX,
            MPI_COMM_WORLD
        );
        if (rc2 != MPI_SUCCESS) MPI_Abort(MPI_COMM_WORLD, rc2);
        global_best_node = reduced_best_node;

        metrics.messages_sent += num_ranks;
        metrics.bytes_sent    += num_ranks * sizeof(int);

        if (global_best_node == -1) break;

        int u = global_best_node;
        settled[u] = true;

        // all ranks relax edges of u since all have full graph
        for (const auto& [v, w] : g.adj[u]) {
            // inf guard ensures addition cant overflow before we do it
            if (!settled[v] && dist[u] < INF - w && dist[u] + w < dist[v]) {
                dist[v] = dist[u] + w;
            }
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    metrics.runtime_seconds =
        std::chrono::duration<double>(end - start).count();

    if (rank == 0) {
        int reachable = 0;
        for (int i = 0; i < num_nodes; i++) {
            if (dist[i] < INF) reachable++;
        }
        LOG_INFO("[dijkstra] Source: " << source);
        LOG_INFO("[dijkstra] Reachable nodes: " << reachable << " / " << num_nodes);
    }

    return dist;
}