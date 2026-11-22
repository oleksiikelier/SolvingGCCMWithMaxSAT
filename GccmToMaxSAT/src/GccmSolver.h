//
// Created by alex on 19.11.26.
//

#ifndef GCCMTOMAXSAT_GCCMSOLVER_H
#define GCCMTOMAXSAT_GCCMSOLVER_H

#include <EvalMaxSAT/EvalMaxSAT.h>
#include "networkit/Globals.hpp"

#include "BooleanEncoder.h"
#include "networkit/centrality/GroupCloseness.hpp"
#include "../satsuma/src/satsuma.h"

// Wrapper for EvalMaxSAT solver with some additional functionality.
class GccmSolver : public EvalMaxSAT<> {
public:
    using EvalMaxSAT::EvalMaxSAT;

    size_t iteration = 1;
    bool useSymBreaking = true;

    std::unique_ptr<EvalMaxSAT> solver = std::make_unique<EvalMaxSAT>();

    [[nodiscard]] std::vector<NetworKit::node> getSolution(BooleanEncoder const &encoder) const;

    std::vector<Grover::NodeID> getNodesToAdvance(BooleanEncoder &encoder) const;

    // Adds n new variables to the solver
    void addNewVariables(int n) const;

    // Adds soft clauses to the solver.
    // NOTE: currently soft clauses are only used to model the objective function.
    // Each varialbe corresponds to a weighted clause of size 1:
    // x_v_i == ({variable}, weight)
    // So they are stored as a std::pair<int, int> instead of std::pair<std::vector<int32_t>, int>
    void addSoftClauses(std::vector<IntTuple> const &clauses) const;

    void addHardClauses(std::vector<std::vector<int32_t>> const &clauses) const;

    void resetSolver();

    void solve(int k, bool encodeExactAsAtMost, NetworKit::Graph const &originalGraph, BooleanEncoder &encoder);

    void precomputeEccs(NetworKit::Graph const &originalGraph, BooleanEncoder const &encoder);

    static satsuma::SymBreakResult breakSymmetries(int nVars, std::vector<std::vector<int>> &clauses, bool fixMode);

    static void printStats(t_weight cost, t_weight farness, size_t iteration);

    static void printSolution(std::vector<NetworKit::node> const &solution);


private:
    const size_t MAX_NUMBER_OF_VARIABLES = 100'000;
    const size_t MAX_NUMBER_OF_ITERATIONS = 32;

    std::vector<uint64_t> cachedEccs;

};

#endif //GCCMTOMAXSAT_GCCMSOLVER_H
