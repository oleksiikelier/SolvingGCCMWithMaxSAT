//
// Created by alex on 25.11.26.
//

#ifndef GCCMTOMAXSAT_MAXSLIMSOLVER_H
#define GCCMTOMAXSAT_MAXSLIMSOLVER_H
#include "AbstractSolver.h"

class MaxSlimSolver : public AbstractSolver {
public:
    MaxSlimSolver(Configuration const &cfg);

    std::vector<uint64_t> solve(int k, NetworKit::Graph &originalGraph, BooleanEncoder &encoder) override;
    std::vector<Grover::NodeID> getNodesToAdvance(BooleanEncoder &encoder, std::vector<bool> &currentAssignment) override;
    std::vector<uint64_t> getSolution(BooleanEncoder &encoder, std::vector<bool> &currentAssignment) override;

private:

    bool saveWcnf_;
    FILE *f = nullptr;
    char filename[16] = "f.wcnf";
    std::string binary_;
    size_t timeout_;

    [[nodiscard]] std::optional<Solution<bool>> runMaxSlim(BooleanEncoder const &encoder, int iteration) const;
    [[nodiscard]] static std::string saveWcnf(BooleanEncoder const &encoder, int iteration) ;
    void saveWcnf(BooleanEncoder const &encoder) const;
    void prepareTempFile();
    void cleanTempFile() const;
};

#endif //GCCMTOMAXSAT_MAXSLIMSOLVER_H
