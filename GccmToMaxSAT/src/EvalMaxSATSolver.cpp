//
// Created by alex on 27.11.26.
//

#include "EvalMaxSATSolver.h"

#include "symmetry.h"
#include "networkit/centrality/GroupCloseness.hpp"

EvalMaxSATSolver::EvalMaxSATSolver(Configuration const &cfg) {
    this->encodeExactAsAtMost_ = cfg.encodeExactAsAtMost;
    this->maxIterations = cfg.maxNumberOfIterations;
    this->useSymBreaking_ = cfg.useSymBreaking;
    prepareLoggingTalbe();
}

void EvalMaxSATSolver::addHardClauses(std::vector<std::vector<int32_t>> const &clauses) const {

    for (const auto & clause: clauses) {
        this->solver_->addClause(clause);
    }

}

void EvalMaxSATSolver::addSoftClauses(std::vector<IntTuple> const &clauses) const{

    for (const auto & weightedClause: clauses) {

        if (weightedClause.second == 0) {
            continue;
        }

        this->solver_->addClause({weightedClause.first}, weightedClause.second);
    }

}

void EvalMaxSATSolver::addNewVariables(const int n) const {

    if (n <= 0 || n > MAX_NUMBER_OF_VARS) {
        std::cerr << "Error: n should be between 0 < n < " << MAX_NUMBER_OF_VARS << ".\n";
        exit(-1);
    }

    for (int i = 0; i < n; i++) {
        this->solver_->newVar();
    }

}

std::vector<uint64_t> EvalMaxSATSolver::solve(int k, NetworKit::Graph &originalGraph, BooleanEncoder &encoder) {

    std::vector<uint64_t> solution;
    int iteration = 1;

    // Needed for f(S) computation
    NetworKit::GroupCloseness gc(originalGraph);

    // Instead of computing it in every iteration, precompute once and use them later.
    // Shouldn't cause any problems since the graph itself does not change during the loop.
    precomputeEccs(originalGraph, encoder);

    while (iteration < this->maxIterations) {
        TimePoint time = Clock::now();
        encoder.encode(k, this->encodeExactAsAtMost_);
        auto encodingTime = elapsedSeconds(time, Clock::now());

        int n = encoder.getNumberOfUsedVarialbes();
        std::cout << MAGENTA "(EvalMaxSATSolver)" RESET " Creating " << n << " new variables\n" << std::endl;

        addNewVariables(n);

        // Assert last used variable id is correct
        int maxId = 0;
        for (auto& c : encoder.hardClauses) for (int l : c) maxId = std::max(maxId, std::abs(l));
        for (auto &key: encoder.softClauses | std::views::keys) maxId = std::max(maxId, std::abs(key));
        assert(maxId <= n - 1);

        if (this->useSymBreaking_) {

            TimePoint breakingStart = Clock::now();

            auto symBreak = run_satsuma_maxsat(n, encoder.hardClauses, encoder.softClauses, -1);
            int variablesDelta = symBreak.nVars - n;
            auto clausesDelta =  symBreak.container.size() - encoder.hardClauses.size();

            std::cout <<  CYAN "(Satsuma)" RESET << " Number of variables before symmetry breaking: " << n << std::endl;
            std::cout << CYAN "(Satsuma)" RESET " Number of variables after symmetry breaking: " << symBreak.nVars;
            std::cout << GREEN " (+" << variablesDelta << ")\n"  RESET << std::endl;

            std::cout << CYAN "(Satsuma)" RESET " Number of clauses before symmetry breaking: " << encoder.hardClauses.size() << std::endl;
            std::cout << CYAN "(Satsuma)" RESET " Number of clauses after symmetry breaking: " << symBreak.container.size();
            std::cout << GREEN " (+" << clausesDelta << ")\n"  RESET << std::endl;

            std::cout
            << CYAN "(Satsuma)" RESET  " Time taken: "
            << elapsedSeconds(breakingStart, Clock::now()) << "(s)\n" << std::endl;

            if (variablesDelta > 0) {
                addNewVariables(variablesDelta);

                std::cout << MAGENTA "(EvalMaxSatSolver)" RESET
                << " Adding additional "
                << variablesDelta
                << " variables after symmetry breaking."
                << std::endl;

            }

            std::cout << MAGENTA "(EvalMaxSatSolver)" RESET " Adding " << symBreak.container.size() << " hard clauses" << std::endl;
            addHardClauses(symBreak.container);
        }
        else
        {
            std::cout << MAGENTA "(EvalMaxSatSolver)" RESET " Adding " << encoder.hardClauses.size() << " hard clauses" << std::endl;
            addHardClauses(encoder.hardClauses);
        }


        std::cout
        << MAGENTA "(EvalMaxSatSolver)" RESET " Adding "
        << encoder.softClauses.size()
        << " soft clauses\n"
        << std::endl;

        addSoftClauses(encoder.softClauses);

        time = Clock::now();
        bool result = this->solver_->solve();
        auto solvingTime = elapsedSeconds(time, Clock::now());

        if (!result) {
            std::cout << RED " \nCurrent instrance of the problem is UNSAT.\n" RESET << std::endl;
            exit(-1);
        }

        auto assignment = extractAssignment(encoder);
        solution = getSolution(encoder, assignment);
        auto nodesToAdvance = getNodesToAdvance(encoder, assignment);
        auto farness = static_cast<t_weight>(gc.computeFarness(solution));

        loggingTable.add_row({
            std::to_string(iteration),
            std::to_string(farness),
            std::to_string(this->solver_->getCost()),
            std::to_string(encoder.firstConstraintSize),
            std::to_string(encoder.secondConstraintSize),
            std::to_string(encoder.thirdConstraintSize),
            std::to_string(encoder.hardClauses.size()),
            std::to_string(encoder.softClauses.size()),
            std::to_string(encodingTime),
            std::to_string(solvingTime)
        });

        if (nodesToAdvance.empty() || this->solver_->getCost() == farness) {
            std::cout << MAGENTA "\n(EvalMaxSatSolver)" RESET " Solution is optimal." << std::endl;
            return solution;
        }

        std::cout
        << MAGENTA "(EvalMaxSatSolver)" RESET
        << "Current solution is not optimal. Size of the nodesToAdvance: "
        << nodesToAdvance.size()
        << std::endl;

        std :: cout << "\n----------------------------------------------\n" << std::endl;
        iteration++;

        encoder.graph_.advanceAllBFS(nodesToAdvance);
        encoder.resetEncoder();
        nodesToAdvance.clear();
        resetSolver();

    }

    std::cout
    << YELLOW "\n(Warning):" RESET
    "Iteration limit is exceeded. Best found solution is not optimal. Consider increasing allowed number of iterations."
    << std::endl;

    return solution;
}

std::vector<Grover::NodeID> EvalMaxSATSolver::getNodesToAdvance(BooleanEncoder &encoder, std::vector<bool> &currentAssignment) {
    std::vector<Grover::NodeID> nodesToAdvance;

    for (int v = 0; v < encoder.graph_.r_n; ++v) {

        auto dv = encoder.graph_.d(v);
        auto lit = encoder.variables.at({v, dv});
        auto ecc = cachedEccs[encoder.graph_.translate[v]];

        // Early exit condintion: x_v_dv = 1 && ecc > d(v)
        if (currentAssignment[lit] && ecc > dv) {
            nodesToAdvance.push_back(v);
        }
    }

    return nodesToAdvance;
}

std::vector<NetworKit::node> EvalMaxSATSolver::getSolution(BooleanEncoder &encoder, std::vector<bool> &currentAssignment){

    std::vector<NetworKit::node> solution;

    for (const auto &mappedVariable: encoder.variables) {

        // mappedVariable maps variables x_v_i to a unique variable inside the solver.
        //
        // Find all x_v_0 variables which value inside the solver is equal to 1.
        const int v = mappedVariable.first.first;
        const int i = mappedVariable.first.second;
        if (i == 0 && currentAssignment[mappedVariable.second]) {
            solution.push_back(encoder.graph_.translate[v]);
        }
    }

    return solution;
}


void EvalMaxSATSolver::resetSolver() {
    solver_ = std::make_unique<EvalMaxSAT<>>();
}

std::vector<bool> EvalMaxSATSolver::extractAssignment(BooleanEncoder const &encoder) const {

    std::vector<bool> assignment(encoder.variables.size() + 1, false);

    // Variables start from 1, so instead of doing -1, the size of the vector is 1 bigger, to avoid indexing mistakes.

    for (const auto &val: encoder.variables | std::views::values) {
        assignment[val] = this->solver_->getValue(val);
    }

    return assignment;
}
