//
// Created by alex on 10.11.26.
//

#include "BooleanEncoder.h"
#include "utils.h"

BooleanEncoder::BooleanEncoder(const Grover::Graph &graph, bool verbose) {
    graph_ = graph;
    verbose_ = verbose;
}

//______________________________________________________________________________________________________________________

void BooleanEncoder::preprocessGraph(NetworKit::Graph &nkGraph, const int k, bool useHeuristic) {

    auto suff_set = graph_.get_in_suff_set();

    auto reduced = graph_.compute_reduction(nkGraph);
    new_in_suff_set.assign(graph_.r_n, false);
    for (int v = 0; v < graph_.r_n; ++v) {
        new_in_suff_set[v] = suff_set[graph_.translate[v]];
    }

    std::vector<int> initial_d(graph_.n, 2);

    if (useHeuristic) {
        Solution<int> heuristic_solution = computeHeuristicSolution(graph_, nkGraph, k);
        std::vector<int> dists = graph_.get_min_dist_to(heuristic_solution.vertex_set);
        initial_d = computeInitialDistances(dists);
    }

    // Compute intial distances and all vertices available from the chosen vertex with specific distance.
    const TimePoint dist_start = Clock::now();
    graph_.compute_initial_distances(initial_d);

    std::cout
    << "\nInitial Distance Time (Seconds): "
    << elapsedSeconds(dist_start, Clock::now())
    << "\n"
    << std::endl;

    mapVariables();
}

//______________________________________________________________________________________________________________________

void BooleanEncoder::mapVariables() {

    // for (int v = 0; v < graph_.r_n; ++v) ---> for each v in V \ D
    // for (int v = 0; v < graph_.r_n; ++v)  and if (!new_in_suff_set[v]) ---> for each v in V \ A

    for (int v = 0; v < graph_.r_n; ++v) {

        int i = 0;

        if (!new_in_suff_set[v]) {
            i = 1;
        }

        for (; i <= graph_.d(v); ++i) {

            variables.insert({std::pair(v, i), lastXVar});
            lastXVar++;

        }
    }

    nextVar = lastXVar;
}

//______________________________________________________________________________________________________________________

void BooleanEncoder::encodeObjectiveFunction() {

    for (int v = 0; v < graph_.r_n; ++v) {

        int i = 0;

        if (!new_in_suff_set[v]) {
            i = 1;
        }

        for (; i <= graph_.d(v); ++i) {
            softClauses.emplace_back(-variables.at({v, i}), graph_.c(v, i));
        }
    }

}

//______________________________________________________________________________________________________________________

void BooleanEncoder::encodeExactlyOneConstraint() {
    const TimePoint encodingStart = Clock::now();
    std::vector<int32_t> literals;
    for (int v = 0; v < graph_.r_n; ++v)
    {
        int i = 0;

        if (!new_in_suff_set[v]) {
            i = 1;
        }

        for (; i <= graph_.d(v); ++i) {
            literals.push_back(variables.at({v, i}));
        }

        if (literals.empty()) {
            continue;
        }

        hardClauses.push_back(literals);
        if (literals.size() > 1) {
            nextVar = pb2_cnf.encodeAtMostK(literals, 1, hardClauses, nextVar) + 1;
        }
        literals.clear();
    }

    if (verbose_) {
        std::cout
        << "Encoding of the ExactlyOne constraint (Seconds): "
        << elapsedSeconds(encodingStart, Clock::now())
        << "\n"
        << std::endl;

        std::cout
        << "(encodeExactlyOneConstraint) Number of hard clauses so far: "
        << hardClauses.size()
        << std::endl;

    }


}

//______________________________________________________________________________________________________________________

void BooleanEncoder::encodeExactlyKConstraint(const int k) {

    if (k <= 0 || k > graph_.get_num_verts()) {
        std::cerr << "Error: wrong k value. k must be in range 0 < k < n" << std::endl;
    }

    const TimePoint encodingStart = Clock::now();
    std::vector<int32_t> literals;

    for (int v = 0; v < graph_.r_n; ++v) {

        if (!new_in_suff_set[v]) {
            continue;
        }

        literals.push_back(variables.at({v, 0}));
    }


    nextVar = pb2_cnf.encodeAtMostK(literals, k, hardClauses, nextVar) + 1;
    nextVar = pb2_cnf.encodeAtLeastK(literals, k, hardClauses, nextVar) + 1;

    literals.clear();

    if (verbose_) {
        std::cout
        << "Encoding of the ExactlyK (k = " << k << ") constraint: (Seconds): "
        << elapsedSeconds(encodingStart, Clock::now())
        << "\n"
        << std::endl;

        std::cout
        << "(encodeExactlyKConstraint) Number of hard clauses so far: "
        << hardClauses.size()
        << std::endl;

    }

}

void BooleanEncoder::encodeAtLeastOneConstraint() {

    const TimePoint encodingStart = Clock::now();

    // x_{v, i} -> (x_{w1, 0} v x_{w2_0} v ...)  <=> -x_{v, i} v (x_{w1, 0} v x_{w2_0} v ...)
    for (int v = 0; v < graph_.r_n; ++v)
    {
        for (int i = 1; i <= graph_.d(v) - 1; ++i)
        {

            std::vector<int32_t> clause{-variables.at({v, i})};

            for (auto w: graph_.vertsAtDist[v][i])
            {
                w = graph_.rtranslate[w];
                if (w == -1 || !new_in_suff_set[w])
                {
                    continue;
                }
                clause.push_back(variables.at({w, 0}));
            }

            hardClauses.push_back(std::move(clause));

        }
    }

    if (verbose_) {
        std::cout
        << "(encodeAtLeastOneConstraint) Number of hard clauses so far: "
        << hardClauses.size()
        << std::endl;

        std::cout
        << "Encoding of the AtLeastOne constraint (Seconds): "
        << elapsedSeconds(encodingStart, Clock::now())
        << "\n"
        << std::endl;
    }
}

