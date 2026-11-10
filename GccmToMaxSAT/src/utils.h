//
// Created by alex on 10.11.26.
//

#ifndef GCCMTOMAXSAT_UTILS_H
#define GCCMTOMAXSAT_UTILS_H

#include "graph.h"

#include <networkit/graph/Graph.hpp>

#include <chrono>
#include <vector>

using Clock = std::chrono::steady_clock;
using TimePoint = std::chrono::steady_clock::time_point;

template <class T>
struct Solution {
    std::vector<T> vertex_set;
    int score;
};

double elapsedSeconds(TimePoint start, TimePoint end);

NetworKit::Graph convertToNetworKit(const Grover::Graph &graph);

Grover::Graph loadGraph(const std::string &path);

Solution<int> computeHeuristicSolution(Grover::Graph &graph, const NetworKit::Graph &nk_graph, int k);

std::vector<int> computeInitialDistances(const std::vector<int> &dists);

#endif //GCCMTOMAXSAT_UTILS_H
