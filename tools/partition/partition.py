#!/usr/bin/env python3
import json
import argparse
import random

def partition_graph(graph_path, num_ranks, output_path, strategy='contiguous', seed=42):
    with open(graph_path, 'r') as f:
        graph = json.load(f)
    nodes = graph['nodes']
    num_nodes = graph['num_nodes']

    ownership = {}

    if strategy == 'contiguous':
        for node in nodes:
            nid = node['id']
            rank = min(nid * num_ranks // num_nodes, num_ranks - 1)
            ownership[str(nid)] = rank

    elif strategy == 'random':
        rng = random.Random(seed)
        node_ids = [node['id'] for node in nodes]
        rng.shuffle(node_ids)
        for i, nid in enumerate(node_ids):
            rank = min(i * num_ranks // num_nodes, num_ranks - 1)
            ownership[str(nid)] = rank

    else:
        raise ValueError(f"Unknown strategy: {strategy}")

    partition = {
        "num_ranks": num_ranks,
        "num_nodes": num_nodes,
        "seed": seed,
        "strategy": strategy,
        "ownership": ownership
    }

    with open(output_path, 'w') as f:
        json.dump(partition, f, indent=2)

    print(f"[{strategy}] Partitioned {num_nodes} nodes across {num_ranks} ranks")
    print(f"Seed: {seed}")
    print(f"Saved to {output_path}")

    counts = [0] * num_ranks
    for rank in ownership.values():
        counts[rank] += 1
    for r, c in enumerate(counts):
        print(f"  Rank {r}: {c} nodes")

    # Print edge cut stats
    edge_cuts = 0
    for edge in graph['edges']:
        src_rank = ownership[str(edge['src'])]
        dst_rank = ownership[str(edge['dst'])]
        if src_rank != dst_rank:
            edge_cuts += 1
    print(f"  Edge cuts: {edge_cuts} / {len(graph['edges'])} ({100*edge_cuts/len(graph['edges']):.1f}%)")

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Partition graph across MPI ranks')
    parser.add_argument('graph', help='Path to graph.json')
    parser.add_argument('--ranks', type=int, required=True, help='Number of MPI ranks')
    parser.add_argument('--out', required=True, help='Output partition file path')
    parser.add_argument('--strategy', choices=['contiguous', 'random'], default='contiguous')
    parser.add_argument('--seed', type=int, default=42, help='Random seed (for random strategy)')
    args = parser.parse_args()

    partition_graph(args.graph, args.ranks, args.out, args.strategy, args.seed)