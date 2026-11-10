#include "BooleanEncoder.h"
#include "utils.h"

#include "graph.h"

#include "EvalMaxSAT/EvalMaxSAT.h"

#include <networkit/graph/Graph.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

int main(int argc, char *argv[]) {


    if (argc != 3) {
        std::cerr << "Usage: ./main <graph_file> k" << std::endl;
        return -1;
    }

    // Load initial graph and create a NetworKit version of it.
    const int k = std::stoi(argv[2]);
    Grover::Graph graph = loadGraph(argv[1]);
    NetworKit::Graph graphNk = convertToNetworKit(graph);

    EvalMaxSAT solver;
    BooleanEncoder encoder(graph, true);

    encoder.preprocessGraph(graphNk, k);

    encoder.encodeObjectiveFunction();

    // (5a)
    encoder.encodeExactlyKConstraint(k);

    // (5b)
    encoder.encodeExactlyOneConstraint();

    // (5c)
    encoder.encodeAtLeastOneConstraint();

    std::cout << "Last used variable: " << encoder.nextVar << std::endl;

    for (int i = 1; i < encoder.nextVar; i++) {
        solver.newVar();
    }


    std::cout << "\nAdding soft clauses" << std::endl;
    for (const auto & pair: encoder.softClauses) {
        solver.addClause({pair.first}, pair.second);
    }

    std::cout << "\nAdding hard clauses" << std::endl;
    for (const auto & clause: encoder.hardClauses) {
        solver.addClause(clause);
    }

    if (!solver.solve()) {
        std::cout << "\nRESULT: UNSAT" << std::endl;
    }

    else {
        std::cout << "\nRESULT: Solution found" << std::endl;
        std::cout << "\nCost: " << solver.getCost() << std::endl;
    }

}