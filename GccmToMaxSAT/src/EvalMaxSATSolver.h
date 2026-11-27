//
// Created by alex on 27.11.26.
//

#ifndef GCCMTOMAXSAT_EVALMAXSATSOLVER_H
#define GCCMTOMAXSAT_EVALMAXSATSOLVER_H
#include "AbstractSolver.h"
#include "EvalMaxSAT/EvalMaxSAT.h"

class EvalMaxSATSolver : public AbstractSolver {
public:
    EvalMaxSATSolver(Configuration const &cfg);

    std::vector<uint64_t> solve(int k, NetworKit::Graph &originalGraph, BooleanEncoder &encoder) override;
    std::vector<Grover::NodeID> getNodesToAdvance(BooleanEncoder &encoder, std::vector<bool> &currentAssignment) override;
    std::vector<uint64_t> getSolution(BooleanEncoder &encoder, std::vector<bool> &currentAssignment) override;

    [[nodiscard]] std::vector<bool> extractAssignment(BooleanEncoder const &encoder) const;

    void addNewVariables(int n) const;
    void addSoftClauses(std::vector<IntTuple> const &clauses) const;
    void addHardClauses(std::vector<std::vector<int32_t>> const &clauses) const;
    void resetSolver();


private:
    std::unique_ptr<EvalMaxSAT<>> solver_ = std::make_unique<EvalMaxSAT<>>();
    bool useSymBreaking_ = false;


    const size_t MAX_NUMBER_OF_VARS = 100'000;
};

#endif //GCCMTOMAXSAT_EVALMAXSATSOLVER_H
