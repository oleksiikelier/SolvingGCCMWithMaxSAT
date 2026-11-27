//
// Created by alex on 10.11.26.
//

#include "BooleanEncoder.h"
#include "utils.h"

BooleanEncoder::BooleanEncoder(const Grover::Graph &graph, EncodingsTuple const &encodings, bool verbose, const std::string &graphName) {

    this->graph_ = graph;
    this->verbose_ = verbose;
    this->graphName_ = graphName;

    pbLibConfig = std::make_shared<PBConfigClass>();
    pbLibConfig->amk_encoder = encodings.first;
    pbLibConfig->amo_encoder = encodings.second;

    pb2cnf = std::make_unique<PB2CNF>(pbLibConfig);
};
//______________________________________________________________________________________________________________________

void BooleanEncoder::resetEncoder() {
    lastXVar = 1;
    nextVar = 1;

    hardClauses.clear();
    softClauses.clear();
    //new_in_suff_set.clear();
    variables.clear();
    pb2cnf = std::make_unique<PB2CNF>(pbLibConfig);

    mapVariables();
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
        std::vector<int> dists = graph_.get_min_dist_to(heuristic_solution.container);
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
    const TimePoint encodingStart = Clock::now();

    for (int v = 0; v < graph_.r_n; ++v) {

        int i = 0;

        if (!new_in_suff_set[v]) {
            i = 1;
        }

        for (; i <= graph_.d(v); ++i) {
            auto weight = graph_.c(v, i);
            if (weight== 0) {
                continue;
            }
            softClauses.emplace_back(-variables.at({v, i}), graph_.c(v, i));
        }
    }

    if (verbose_) {
        std::cout
        << GREEN "(encodeObjectiveFunction)" RESET << " Time: "
        << elapsedSeconds(encodingStart, Clock::now())
        << " (s)"
        << std::endl;

        std::cout
        << GREEN "(encodeObjectiveFunction)" RESET " Number of soft clauses after encoding: "
        << softClauses.size()
        << "\n"
        << std::endl;
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
            nextVar = pb2cnf->encodeAtMostK(literals, 1, hardClauses, nextVar) + 1;
        }
        literals.clear();
    }

    if (verbose_) {
        std::cout
        << GREEN "(encodeExactlyOneConstraint)" RESET " Time: "
        << elapsedSeconds(encodingStart, Clock::now())
        << " (s)"
        << std::endl;

        std::cout
        << GREEN "(encodeExactlyOneConstraint)" RESET " Number of hard clauses after encoding: "
        << hardClauses.size()
        << "\n"
        << std::endl;
    }


}
//______________________________________________________________________________________________________________________

void BooleanEncoder::encodeAtMostKConstraint(const int k) {
    const TimePoint encodingStart = Clock::now();
    std::vector<int32_t> literals;

    for (int v = 0; v < graph_.r_n; ++v) {

        if (!new_in_suff_set[v]) {
            continue;
        }

        literals.push_back(variables.at({v, 0}));
    }

    nextVar = pb2cnf->encodeAtMostK(literals, k, hardClauses, nextVar) + 1;
    literals.clear();

    if (verbose_) {
        std::cout
        << GREEN "(encodeAtMostKConstraint)" RESET " (k = " << k << ") Time: "
        << elapsedSeconds(encodingStart, Clock::now())
        << " (s)"
        << std::endl;

        std::cout
        << GREEN "(encodeAtMostKConstraint)" RESET " Number of hard clauses after encoding: "
        << hardClauses.size()
        << "\n"
        << std::endl;

    }


}


//______________________________________________________________________________________________________________________

void BooleanEncoder::encodeExactlyKConstraint(const int k) {

    const TimePoint encodingStart = Clock::now();
    std::vector<int32_t> literals;

    for (int v = 0; v < graph_.r_n; ++v) {

        if (!new_in_suff_set[v]) {
            continue;
        }

        literals.push_back(variables.at({v, 0}));
    }


    nextVar = pb2cnf->encodeAtMostK(literals, k, hardClauses, nextVar) + 1;
    nextVar = pb2cnf->encodeAtLeastK(literals, k, hardClauses, nextVar) + 1;

    literals.clear();

    if (verbose_) {
        std::cout
        << GREEN "(encodeExactlyKConstraint)" RESET " (k = " << k << ") Time: "
        << elapsedSeconds(encodingStart, Clock::now())
        << " (s)"
        << std::endl;

        std::cout
        << GREEN "(encodeExactlyKConstraint)" RESET " Number of hard clauses after encoding: "
        << hardClauses.size()
        << "\n"
        << std::endl;

    }

}
//______________________________________________________________________________________________________________________

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
        << GREEN"(encodeAtLeastOneConstraint)" RESET " Time: "
        << elapsedSeconds(encodingStart, Clock::now())
        << " (s)"
        << std::endl;

        std::cout
        << GREEN "(encodeAtLeastOneConstraint)" RESET " Number of hard clauses after encoding: "
        << hardClauses.size()
        << "\n"
        << std::endl;

    }
}

//______________________________________________________________________________________________________________________
std::vector<int32_t> BooleanEncoder::getVariables() {
    std::vector<int32_t> obj;
    obj.reserve(variables.size());

    for (const auto &[k, v] : variables) {
        obj.push_back(v);
    }

    return obj;
}
//______________________________________________________________________________________________________________________
int BooleanEncoder::getNumberOfUsedVarialbes() const{
    return nextVar;
}
//______________________________________________________________________________________________________________________
void BooleanEncoder::saveHardClauses(const std::string &fileName) {

    std::ofstream file(fileName);

    if (!file.is_open()) {
        std::cerr << "Error: cannot open file " << fileName << "." << std::endl;
    }

    file << "p cnf " << nextVar - 1 << " " << hardClauses.size() << "\n";

    for (const auto &clause : hardClauses) {
        for (const auto &lit : clause) {
            file << lit << " ";
        }
        file << 0 << "\n";
    }

    file.close();
}

void BooleanEncoder::encode(int k, bool encodeExactAsAtMost) {
    encodeObjectiveFunction();
    objectiveSize = this->softClauses.size();

    // (5a)
    if (encodeExactAsAtMost) {
        // Alternative encoding as in Grover/src/iterative.h (5a with <=k instead of =k)
        encodeAtMostKConstraint(k);
    }
    else {
        encodeExactlyKConstraint(k);
    }

    firstConstraintSize = this->hardClauses.size();

    // (5b)
    encodeExactlyOneConstraint();
    secondConstraintSize = this->hardClauses.size() - firstConstraintSize;
    // (5c)
    encodeAtLeastOneConstraint();
    thirdConstraintSize = this->hardClauses.size() - secondConstraintSize - firstConstraintSize;
}

























