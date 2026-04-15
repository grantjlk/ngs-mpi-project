#include <mpi.h>
#include <iostream>
#include <string>
#include <climits>
#include "graph.h"
#include "partition.h"
#include "metrics.h"
#include "leader_election.h"
#include "dijkstra.h"


int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // parse CLI arguments
    std::string graph_path, part_path, algo;
    int source = 0;
    int rounds = 200;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--graph" && i + 1 < argc)       graph_path = argv[++i];
        else if (arg == "--part" && i + 1 < argc)   part_path  = argv[++i];
        else if (arg == "--algo" && i + 1 < argc)   algo       = argv[++i];
        else if (arg == "--source" && i + 1 < argc) source     = std::stoi(argv[++i]);
        else if (arg == "--rounds" && i + 1 < argc) rounds     = std::stoi(argv[++i]);
    }

    if (graph_path.empty() || part_path.empty() || algo.empty()) {
        if (rank == 0) {
            std::cerr << "Usage: ngs_mpi --graph <path> --part <path> "
                      << "--algo <leader|dijkstra> [--source 0] [--rounds 200]\n";
        }
        MPI_Finalize();
        return 1;
    }

    if (algo != "leader" && algo != "dijkstra") {
        if (rank == 0) {
            std::cerr << "Error: unknown algorithm '" << algo
                      << "'. Must be 'leader' or 'dijkstra'\n";
        }
        MPI_Finalize();
        return 1;
    }

    if (rounds <= 0) {
        if (rank == 0) {
            std::cerr << "Error: --rounds must be a positive integer\n";
        }
        MPI_Finalize();
        return 1;
    }

    // load graph and partition on all ranks
    Graph g;
    Partition p;
    try {
        g = load_graph(graph_path);
        p = load_partition(part_path);
    } catch (const std::exception& ex) {
        std::cerr << "[rank " << rank << "] Error loading files: "
                  << ex.what() << "\n";
        MPI_Finalize();
        return 1;
    }

    if (p.num_ranks != size) {
        if (rank == 0) {
            std::cerr << "Error: partition has " << p.num_ranks
                      << " ranks but MPI was launched with " << size << "\n";
        }
        MPI_Finalize();
        return 1;
    }

    if (p.num_nodes != g.num_nodes) {
        if (rank == 0) {
            std::cerr << "Error: partition has " << p.num_nodes
                      << " nodes but graph has " << g.num_nodes << "\n";
        }
        MPI_Finalize();
        return 1;
    }

    if (source < 0 || source >= g.num_nodes) {
        if (rank == 0) {
            std::cerr << "Error: --source " << source
                      << " is out of range [0, " << g.num_nodes - 1 << "]\n";
        }
        MPI_Finalize();
        return 1;
    }

    if (rank == 0) {
        std::cout << "Running algorithm: " << algo
                  << " with " << size << " ranks\n";
    }

    // leader election algorithm
    Metrics metrics = {0, 0, 0, 0.0};

    if (algo == "leader") {
        int leader = run_leader_election(g, p, rank, size, rounds, metrics);
        if (rank == 0) {
            print_metrics("Leader Election", metrics);
            // verify all ranks agree
            std::cout << "[leader] All nodes agree on leader: " << leader << "\n";
        }
    }
    else if (algo == "dijkstra") {
    std::vector<int> dist = run_dijkstra(g, p, source, rank, size, metrics);
    if (rank == 0) {
        print_metrics("Dijkstra", metrics);
        // print first 10 distances as sanity check
        std::cout << "[dijkstra] Sample distances from source " << source << ":\n";
        for (int i = 0; i < std::min(10, g.num_nodes); i++) {
            if (dist[i] < (INT_MAX / 2)) {
                std::cout << "  node " << i << ": " << dist[i] << "\n";
            } else {
                std::cout << "  node " << i << ": unreachable\n";
            }
        }
    }
}

    MPI_Finalize();
    return 0;
}