#include "BooleanEncoder.h"
#include "utils.h"

#include "graph.h"

#include "EvalMaxSAT/EvalMaxSAT.h"

#include <networkit/graph/Graph.hpp>
#include <networkit/distance/Eccentricity.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "networkit/centrality/GroupCloseness.hpp"
#include "networkit/distance/MultiTargetBFS.hpp"

static void parseArguments(int argc, char *argv[], const PBConfig &config) {

    for (int i = 3; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "--amk") {

            std::string amkEncoder = argv[++i];

            if (amkEncoder == "card") {
                config->amk_encoder = AMK_ENCODER::CARD;
            }

            else if (amkEncoder == "bdd") {
                config->amk_encoder = AMK_ENCODER::BDD;
            }

            else {
                std::cerr << "Error: unknown encoding type for the AMK constraint: " << amkEncoder << std::endl;
                exit(-1);
            }

            std::cout << "AMK encoder: " << amkEncoder << std::endl;
        }

        else if (arg == "--amo") {

            std::string amoEncoder = argv[++i];

            if (amoEncoder == "bdd") {
                config->amo_encoder = AMO_ENCODER::BDD;
            }
            else if (amoEncoder == "kproduct") {
                config->amo_encoder = AMO_ENCODER::KPRODUCT;
            }
            else if (amoEncoder == "bimmander") {
                config->amo_encoder = AMO_ENCODER::BIMANDER;
            }
            else if (amoEncoder == "commander") {
                config->amo_encoder = AMO_ENCODER::COMMANDER;
            }
            else if (amoEncoder == "binary") {
                config->amo_encoder = AMO_ENCODER::BINARY;
            }
            else if (amoEncoder == "nested") {
                config->amo_encoder = AMO_ENCODER::NESTED;
            }
            else if (amoEncoder == "pairwise") {
                config->amo_encoder = AMO_ENCODER::PAIRWISE;
            }
            else {
                std::cerr << "Error: unknown encoding type for the AMO constraint: " << amoEncoder << std::endl;
                exit(-1);
            }

            std::cout << "AMO encoder: " << amoEncoder << std::endl;
        }
    }
}


int main(int argc, char *argv[]) {

    if (argc < 3) {
        std::cerr << "Usage: ./main <graph_file> k" << std::endl;
        return -1;
    }
    std :: cout << "\n-----------Loading and prepocessing-----------\n" << std::endl;

    // Load initial graph and create a NetworKit version of it.
    // Also creates groupCloseness object to compute f(S) later.
    Grover::Graph graph = loadGraph(argv[1]);
    NetworKit::Graph graphNk = convertToNetworKit(graph);
    NetworKit::GroupCloseness gc(graphNk);

    int iteration = 1;
    std::vector<Grover::NodeID> nodesToAdvance;

    const int k = std::stoi(argv[2]);
    if (k <= 0 || k > graph.get_num_verts()) {
        std::cerr << "Error: wrong k value. k must be in range 0 < k < n" << std::endl;
        return -1;
    }

    PBConfig pbLibConfig = std::make_shared<PBConfigClass>();
    parseArguments(argc, argv, pbLibConfig);

    BooleanEncoder encoder(graph, pbLibConfig, true);
    encoder.preprocessGraph(graphNk, k, false);


    std :: cout << "-----------Starting the main loop-----------\n" << std::endl;;

    while (true)
    {
        //EvalMaxSAT solver;
        auto solver = std::make_unique<EvalMaxSAT<>>();

        if (!solver) {
            std::cerr << "Error: cannot create unique_ptr<EvalMaxSAT<>>" << std::endl;
            return -1;
        }

        encoder.encodeObjectiveFunction();
        // (5a)
        encoder.encodeExactlyKConstraint(k);
        // (5b)
        encoder.encodeExactlyOneConstraint();
        // (5c)
        encoder.encodeAtLeastOneConstraint();

        std::cout << "Last used variable: " << encoder.nextVar << std::endl;
        for (int i = 1; i < encoder.nextVar; i++) {
            solver->newVar();
        }

        std::cout << "\nAdding soft clauses" << std::endl;
        for (const auto & pair: encoder.softClauses) {
            solver->addClause({pair.first}, pair.second);
        }

        std::cout << "\nAdding hard clauses\n" << std::endl;
        for (const auto & clause: encoder.hardClauses) {
            solver->addClause(clause);
        }

        TimePoint solvingStart = Clock::now();

        if (!solver->solve()) {
            std::cout << "\nRESULT: UNSAT" << std::endl;
            return - 1;
        }
        std::cout << "Time: " << elapsedSeconds(solvingStart, Clock::now()) << "(s)" << std::endl;

        std::cout << "Current solution: ";
        // Solution found in the current iteration:  x_v_0 = 1 for all v in V \ A.
        std::vector<NetworKit::node> S;
        for (const auto &mappedVariable: encoder.variables) {

            // mappedVariable maps variables x_v_i to a unique variable inside the solver.
            //
            // Find all x_v_0 variables which value inside the solver is equal to 1.
            const int v = mappedVariable.first.first;
            const int i = mappedVariable.first.second;
            if (i == 0 && solver->getValue(mappedVariable.second)) {
                std::cout << "x_" << encoder.graph_.translate[v] << "_" << i << " ";
                S.push_back(encoder.graph_.translate[v]);
            }

        }

        // Computes f(S) (returned double value is casted into unsinged long long since the graph is unweighted)
        auto f = static_cast<t_weight>(gc.computeFarness(S));

        std::cout << "\nModel cost: " << solver->getCost() << std::endl;
        std::cout << "f(S): " << f << std::endl;
        std::cout << "Iteration: " << iteration << std::endl;
        std::cout << std::endl;


        // Check if solution is optimal by checking that
        // x_v_d(v) = 1 AND ecc(v) > d(v) for any v in V \ A.
        for (int v = 0; v < encoder.graph_.r_n; ++v) {

            auto [furthest, ecc] = NetworKit::Eccentricity::getValue(graphNk, encoder.graph_.translate[v]);
            const int dv = encoder.graph_.d(v);
            if (solver->getValue(encoder.variables.at({v, dv})) && ecc > dv) {
                nodesToAdvance.push_back(v);
            }
        }

        // Current solution is optimal
        if (nodesToAdvance.empty() || solver->getCost() == f) {
            std::cout << "\nSolution is optimal." << std::endl;
            break;
        }

        std::cout << "Current solution is not optimal.\nSize of the nodesToAdvance: " << nodesToAdvance.size() << std::endl;
        encoder.graph_.advanceAllBFS(nodesToAdvance);

        iteration++;
        nodesToAdvance.clear();
        encoder.resetEncoder(pbLibConfig);
        encoder.mapVariables();


        std::cout << "\n--------------------------------\n\n";
    }



}