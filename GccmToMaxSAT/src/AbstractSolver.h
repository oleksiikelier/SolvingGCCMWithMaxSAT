//
// Created by alex on 25.11.26.
//

#ifndef GCCMTOMAXSAT_ABSTRACTSOLVER_H
#define GCCMTOMAXSAT_ABSTRACTSOLVER_H
#include "BooleanEncoder.h"
#include "utils.h"
#include "networkit/graph/AdjListGraph.hpp"
#include "tabulate/table.hpp"

class AbstractSolver {

public:
    virtual ~AbstractSolver() = default;

    virtual std::vector<uint64_t> solve(int k, NetworKit::Graph &originalGraph, BooleanEncoder &encoder) = 0;
    virtual std::vector<Grover::NodeID> getNodesToAdvance(BooleanEncoder &encoder, std::vector<bool> &currentAssignment) = 0;
    virtual std::vector<uint64_t> getSolution(BooleanEncoder &encoder, std::vector<bool> &currentAssignment) = 0;

    void precomputeEccs(NetworKit::Graph const &originalGraph, BooleanEncoder const &encoder);
    void printLoggingTable();
    void prepareLoggingTalbe();

protected:
    size_t maxIterations = 0;
    bool encodeExactAsAtMost_ = false;

    std::vector<uint64_t> cachedEccs;

    tabulate::Table loggingTable;


};

#endif //GCCMTOMAXSAT_ABSTRACTSOLVER_H
