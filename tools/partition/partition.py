#!/usr/bin/env python3
import json
import argparse
import sys

def partition_graph(graph_path, num_ranks, output_path):
    with open(graph_path, 'r') as f:
        graph = json.load(f)

    nodes = graph['nodes']
    num_nodes = graph['num_nodes']

    # Simple contiguous range partitioning
    # e.g. rank 0 gets nodes 0..N/R-1, rank 1 gets N/R..2N/R-1, etc.
    ownership = {}
    for node in nodes:
        nid = node['id']
        rank = min(nid * num_ranks // num_nodes, num_ranks - 1)
        ownership[str(nid)] = rank

    partition = {
        "num_ranks": num_ranks,
        "num_nodes": num_nodes,
        "ownership": ownership
    }

    with open(output_path, 'w') as f:
        json.dump(partition, f, indent=2)

    print(f"Partitioned {num_nodes} nodes across {num_ranks} ranks")
    print(f"Saved to {output_path}")

    # Print summary of how many nodes per rank
    counts = [0] * num_ranks
    for rank in ownership.values():
        counts[rank] += 1
    for r, c in enumerate(counts):
        print(f"  Rank {r}: {c} nodes")

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Partition graph across MPI ranks')
    parser.add_argument('graph', help='Path to graph.json')
    parser.add_argument('--ranks', type=int, required=True, help='Number of MPI ranks')
    parser.add_argument('--out', required=True, help='Output partition file path')
    args = parser.parse_args()

    partition_graph(args.graph, args.ranks, args.out)
