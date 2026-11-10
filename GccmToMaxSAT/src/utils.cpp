//
// Created by alex on 10.11.26.
//
#include "utils.h"

#include <algorithm>
#include <iostream>
#include <fstream>

#include <networkit/graph/GraphTools.hpp>

#include <networkit/centrality/GroupCloseness.hpp>
#include <networkit/centrality/GroupClosenessGrowShrink.hpp>
#include <networkit/centrality/GroupClosenessLocalSearch.hpp>


double elapsedSeconds(TimePoint start, TimePoint end) {
    return static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count()) / 1e9;
}

NetworKit::Graph convertToNetworKit(const Grover::Graph& graph) {

    NetworKit::Graph nk_graph;

    nk_graph.addNodes(graph.get_num_verts());

    for (const Grover::Edge& edge : graph.adj_array) {
        nk_graph.addEdge(edge.source, edge.target, 1, true);
    }

    std::cout << "NetworKit graph: |V| = " << nk_graph.numberOfNodes()
              << " |E| = " << nk_graph.numberOfEdges() << std::endl;

    return nk_graph;
}

Grover::Graph loadGraph(const std::string &path) {

    std::ifstream stream(path);

    if (!stream.is_open()) {
        std::cerr << "Error: cannot open graph file: " << path << std::endl;
    }

    Grover::Graph graph;
    graph << stream;
    graph.use_redu = true;
    graph.use_suff = true;

    std::cout << "Grover graph: |V| = " << graph.n << " |E| = " << graph.m << std::endl;

    return graph;
}


Solution<int> computeHeuristicSolution(Grover::Graph& graph, const NetworKit::Graph& nk_graph, int k) {
    TimePoint gs_start = Clock::now();
    std::vector<NetworKit::node> initial_group;
    initial_group.assign(k, 0);
    for (int i = 0; i < k; i++) {
        initial_group[i] = i * (graph.n / k);
    }

    NetworKit::GroupClosenessGrowShrink gs(nk_graph, initial_group);
    gs.run();
    std::vector<NetworKit::node> gs_result = gs.groupMaxCloseness();
    std::vector<int> gs_group(gs_result.begin(), gs_result.end());

    std::vector<int> gs_dists = graph.get_min_dist_to(gs_group);
    int gs_score = std::accumulate(gs_dists.begin(), gs_dists.end(), 0);

    std::cout << "GS Score: " << gs_score << "\n";
    std::cout << "GS Time (Seconds): " << elapsedSeconds(gs_start, Clock::now()) << "\n";

    TimePoint ls_start = Clock::now();
    std::vector<int> empty_reduction(graph.n, -1);
    // empty_reduction gives an error
    NetworKit::GroupClosenessLocalSearch ls(nk_graph, gs_result, (empty_reduction.data()), false);
    ls.run();
    std::vector<NetworKit::node> ls_result = ls.groupMaxCloseness();
    std::vector<int> ls_group(ls_result.begin(), ls_result.end());

    std::vector<int> ls_dists = graph.get_min_dist_to(ls_group);
    int ls_score = std::accumulate(ls_dists.begin(), ls_dists.end(), 0);

    std::cout << "LS Score: " << ls_score << "\n";
    std::cout << "LS Time (Seconds): " << elapsedSeconds(ls_start, Clock::now()) << "\n";

    return {ls_group, ls_score};
}


std::vector<int> computeInitialDistances(const std::vector<int>& dists) {
    std::vector<int> initial_d = dists;
    for (int& d : initial_d) {
        d = std::max(d + 1, 2);
    }
    return initial_d;
}
