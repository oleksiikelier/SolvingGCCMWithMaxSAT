//
// Created by alex on 19.11.26.
//
// SILENCE LIBRARY WARNINGS
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wall"
#pragma clang diagnostic ignored "-Wextra"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#endif

#include "GccmSolver.h"
#include "utils.h"
#include "../satsuma/src/satsuma.h"
#include "networkit/distance/Eccentricity.hpp"

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

void GccmSolver::addHardClauses(std::vector<std::vector<int32_t>> const &clauses) const {

    for (const auto & clause: clauses) {
        solver->addClause(clause);
    }

}

void GccmSolver::addSoftClauses(std::vector<IntTuple> const &clauses) const{

    for (const auto & weightedClause: clauses) {
        solver->addClause({weightedClause.first}, weightedClause.second);
    }

}

void GccmSolver::addNewVariables(const int n) const {

    if (n <= 0 || n > this->MAX_NUMBER_OF_VARIABLES) {
        std::cerr << "Error: n should be between 0 < n < " << MAX_NUMBER_OF_VARIABLES << ".\n";
        exit(-1);
    }

    for (int i = 0; i < n; i++) {
        solver->newVar();
    }

}

void GccmSolver::precomputeEccs(NetworKit::Graph const &originalGraph, BooleanEncoder const &encoder) {

    TimePoint eccTime = Clock::now();

    // Original size of the graph should be used, since we can hit
    // a vertex which can have a potentially bigger index in the originalgraph  than in the reduced one
    cachedEccs.reserve(encoder.graph_.n);

    for (int v = 0; v < encoder.graph_.r_n; ++v) {
        auto inOriginalGraph = encoder.graph_.translate[v];
        // Returns std::pair where first is the furthest vertex and second is the distance to this vertex (ecc)
        auto pair = NetworKit::Eccentricity::getValue(originalGraph, inOriginalGraph);

        // unsigned long long is too much for now
        cachedEccs[inOriginalGraph] = pair.second;
    }

    std::cout
    << "Time for computing eccs: "
    << elapsedSeconds(eccTime, Clock::now())
    << "(s)\n"
    << std::endl;
}

void GccmSolver::solve(int k, bool encodeExactAsAtMost, NetworKit::Graph const &originalGraph, BooleanEncoder &encoder) {

    // Needed for f(S) computation
    NetworKit::GroupCloseness gc(originalGraph);

    // Instead of computing it in every iteration, precompute once and use them later.
    // Shouldn't cause any problems since the graph itself does not change during the loop.
    precomputeEccs(originalGraph, encoder);

    std :: cout << "----------- Starting the main loop -----------\n" << std::endl;;

    while (iteration < MAX_NUMBER_OF_ITERATIONS) {
        std :: cout << "---------------- Iteration # " << iteration << " ---------------\n" << std::endl;;

        encoder.encodeObjectiveFunction();

        // (5a)
        if (encodeExactAsAtMost) {
            encoder.encodeAtMostKConstraint(k);
        }
        else {
            encoder.encodeExactlyKConstraint(k);
        }

        // (5b)
        encoder.encodeExactlyOneConstraint();
        // (5c)
        encoder.encodeAtLeastOneConstraint();

        const int n  = encoder.getNumberOfUsedVarialbes();
        std::cout << "Creating " << n << " new variables" << std::endl;
        addNewVariables(n);

        //
        if (useSymBreaking) {

            TimePoint breakingStart = Clock::now();
            auto symBreak = breakSymmetries(n - 1, encoder.hardClauses, false);

            std::cout << "\nNumber of variables before symmetry breaking: " << n - 1 << std::endl;
            std::cout << "Number of clauses before symmetry breaking: " << encoder.hardClauses.size() << std::endl;
            std::cout << "Number of variables after symmetry breaking: " << symBreak.nVars << std::endl;
            std::cout << "Number of clauses after symmetry breaking: " << symBreak.clauses.size() << std::endl;
            std::cout << "Time taken: " << elapsedSeconds(breakingStart, Clock::now()) << "(s)\n" << std::endl;
            std::cout << "Adding " << symBreak.clauses.size() << " hard clauses" << std::endl;

            addHardClauses(symBreak.clauses);
        }
        else {
            std::cout << "\nAdding " << encoder.hardClauses.size() << " hard clauses" << std::endl;
            addHardClauses(encoder.hardClauses);
        }


        std::cout << "\nAdding " << encoder.softClauses.size() << " soft clauses\n" << std::endl;
        addSoftClauses(encoder.softClauses);

        //
        TimePoint solvingStart = Clock::now();
        bool result = solver->solve();
        std::cout << "\nTime: " << elapsedSeconds(solvingStart, Clock::now()) << "(s)\n" << std::endl;

        if (!result) {
            std::cout << "Current instrance of the problem is UNSAT.\n" << std::endl;
            exit(-1);
        }
        //

        std::vector<NetworKit::node> solution = getSolution(encoder);
        std::vector<Grover::NodeID> nodesToAdvance = getNodesToAdvance(encoder);
        auto farness = static_cast<t_weight>(gc.computeFarness(solution));

        printSolution(solution);

        printStats(solver->getCost(), farness, iteration);

        if (nodesToAdvance.empty() || solver->getCost() == farness) {
            std::cout << "Solution is optimal." << std::endl;
            break;
        }

        std::cout
        << "Current solution is not optimal. Size of the nodesToAdvance: "
        << nodesToAdvance.size()
        << std::endl;

        std :: cout << "\n---------------- End of Iteration # " << iteration << " ---------------\n" << std::endl;

        iteration++;

        encoder.graph_.advanceAllBFS(nodesToAdvance);
        encoder.resetEncoder();
        nodesToAdvance.clear();
        resetSolver();
    }

}

std::vector<NetworKit::node> GccmSolver::getSolution(BooleanEncoder const &encoder) const{

    std::vector<NetworKit::node> solution;

    for (const auto &mappedVariable: encoder.variables) {

        // mappedVariable maps variables x_v_i to a unique variable inside the solver.
        //
        // Find all x_v_0 variables which value inside the solver is equal to 1.
        const int v = mappedVariable.first.first;
        const int i = mappedVariable.first.second;
        if (i == 0 && solver->getValue(mappedVariable.second)) {
            solution.push_back(encoder.graph_.translate[v]);
        }
    }

    return solution;
}

std::vector<Grover::NodeID> GccmSolver::getNodesToAdvance(BooleanEncoder &encoder) const {

    std::vector<Grover::NodeID> nodesToAdvance;

    for (int v = 0; v < encoder.graph_.r_n; ++v) {

        auto dv = encoder.graph_.d(v);
        auto lit = encoder.variables.at({v, dv});
        auto ecc = cachedEccs[encoder.graph_.translate[v]];

        // Early exit condintion: x_v_dv = 1 && ecc > d(v)
        if (solver->getValue(lit) && ecc > dv) {
            nodesToAdvance.push_back(v);
        }
    }

    return nodesToAdvance;
}

void GccmSolver::resetSolver() {
    solver = std::make_unique<EvalMaxSAT>();
}

void GccmSolver::printSolution(std::vector<NetworKit::node> const &solution) {
    std::cout << "Solution set: { ";
    for (const auto &variable: solution) {
        std::cout << "x_" << variable << "_0" << " ";
    }
    std::cout << "}";
}

void GccmSolver::printStats(t_weight cost, t_weight farness, size_t iteration) {
    std::cout << "\nModel cost: " << cost << std::endl;
    std::cout << "f(S): " << farness << std::endl;
    std::cout << "Iteration: " << iteration << std::endl;
    std::cout << std::endl;
}

satsuma::SymBreakResult GccmSolver::breakSymmetries(const int nVars, std::vector<std::vector<int>> &clauses, const bool fixMode) {
    satsuma::preprocessor p;

    if (fixMode) {                      // same as `satsuma fix`
        p.set_orbitopal_fixing(true);
        p.set_orbitopal_fixing_only(true);
        p.set_schreier_fixing(true);
        p.set_negation_fixing(true);
        p.set_reorder(true);
        p.set_reorder_cliquer(true);
        p.set_iterate(true);
        p.set_preprocess_cnf_unit(true);
        p.set_preprocess_cnf_pure(true);
        // p.set_add_reduced_as_unit(true);  // see caveat below
    }                                    // else: lex = defaults

    std::ostream null_log(nullptr);      // discard logging
    p.set_log_output(&null_log);

    satsuma::tracker track;              // must be set, track is dereferenced unguarded
    track.h_silent = true;
    p.set_tracker(&track);

    cnf2wl f;

    f.reserve(nVars, static_cast<int>(clauses.size()));
    for (auto& c : clauses) {
        f.add_clause(c);
    }   // may need a mutable copy

    satsuma::SymBreakResult out;
    p.set_result(&out);   // the patch below

    p.preprocess(f);

    return out;

}