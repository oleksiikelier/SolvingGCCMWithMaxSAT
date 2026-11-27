//
// Created by alex on 10.11.26.
//

#ifndef GCCMTOMAXSAT_UTILS_H
#define GCCMTOMAXSAT_UTILS_H

#include "BooleanEncoder.h"

#include "graph.h"

#include <networkit/graph/Graph.hpp>

#include <chrono>
#include <vector>

#define RESET   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */

using Clock = std::chrono::steady_clock;
using TimePoint = std::chrono::steady_clock::time_point;

struct Configuration {

    // General
    bool saveWcnf = false;
    bool encodeExactAsAtMost = false;
    bool useHeuristic = false;
    size_t maxNumberOfIterations = 32;

    // MaxSLIM params
    bool useMaxSLIM = false;
    size_t timeout = 15;
    std::string maxSlimBinaryPath;

    // EvalMaxSAT params
    bool useSymBreaking = false;
    bool verbose = false;
    EncodingsTuple encodings;
};

template <class T> struct Solution {
    std::vector<T> container;
    int score = 0;
    int nVars = 0;
};

const std::string WCNF_FOLDER = "GccmToMaxSAT/wcnfs/";
const std::string GRAPHS_FOLDER = "GccmToMaxSAT/graphs/";
const std::string MAXSLIM_BIN = "./../../testing/supplementary/src/maxslim";

double elapsedSeconds(TimePoint start, TimePoint end);

NetworKit::Graph convertToNetworKit(const Grover::Graph &graph);

Grover::Graph loadGraph(const std::string &path);

Solution<int> computeHeuristicSolution(Grover::Graph &graph, const NetworKit::Graph &nk_graph, int k);

std::vector<int> computeInitialDistances(const std::vector<int> &dists);

#endif //GCCMTOMAXSAT_UTILS_H
