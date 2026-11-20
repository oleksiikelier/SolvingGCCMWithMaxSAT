#include "BooleanEncoder.h"
#include "utils.h"

#include "graph.h"

#include "EvalMaxSAT/EvalMaxSAT.h"

#include <networkit/graph/Graph.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "GccmSolver.h"

struct Configuration {
    // eaam = exact as at most
    bool encodeExactAsAtMost = false;
    bool useHeuristic = false;
    bool verbose = false;
    EncodingsTuple encodings;
};

static Configuration parseArguments(int argc, char *argv[]) {

    Configuration cfg;

    for (int i = 3; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "--amk") {

            std::string amkEncoder = argv[++i];

            if (amkEncoder == "card") {
                cfg.encodings.first = AMK_ENCODER::CARD;
            }

            else if (amkEncoder == "bdd") {
                cfg.encodings.first = AMK_ENCODER::BDD;
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
                cfg.encodings.second = AMO_ENCODER::BDD;
            }
            else if (amoEncoder == "kproduct") {
                cfg.encodings.second = AMO_ENCODER::KPRODUCT;
            }
            else if (amoEncoder == "bimmander") {
                cfg.encodings.second = AMO_ENCODER::BIMANDER;
            }
            else if (amoEncoder == "commander") {
                cfg.encodings.second = AMO_ENCODER::COMMANDER;
            }
            else if (amoEncoder == "binary") {
                cfg.encodings.second = AMO_ENCODER::BINARY;
            }
            else if (amoEncoder == "nested") {
                cfg.encodings.second = AMO_ENCODER::NESTED;
            }
            else if (amoEncoder == "pairwise") {
                cfg.encodings.second = AMO_ENCODER::PAIRWISE;
            }
            else {
                std::cerr << "Error: unknown encoding type for the AMO constraint: " << amoEncoder << std::endl;
                exit(-1);
            }

            std::cout << "AMO encoder: " << amoEncoder << std::endl;
        }

        else if (arg == "--eaam") {
            cfg.encodeExactAsAtMost = true;
        }
        else if(arg == "--heuristic") {
            cfg.useHeuristic = true;
        }
        else if (arg == "--verbose") {
            cfg.verbose = true;
        }
    }

    return cfg;
}

static void printHelp() {
    std::cout << "\n\nUsage:\n";
    std::cout << "./cmake-build-debug/GccmToMaxSAT/GccmToMaxSAT <graph_file> k [OPTIONS]\n\n";
    std::cout << "Options:\n";
    std::cout << "--amk <encoding>      Use <encoding> to encode the AT MOST K constraint.\n";
    std::cout << "                      <encoding> can take following values: card, bdd.\n\n";
    std::cout << "--amo <encoding>      Use <encoding> to encode the AT MOST ONE constraint.\n";
    std::cout << "                      <encoding> can take following values: bdd, bimmander,\n";
    std::cout << "                      commander, binary, nested, pairwise, kproduct\n\n";
    std::cout << "--eaam                Encode =k constraint as <= k.\n\n";
    std::cout << "--heuristic           Instead of setting d(v) = 2, use a heuristic solution as a starting point.\n\n";
    std::cout << "--verbose             Print additional information during the solving process.\n\n";

}

int main(int argc, char *argv[]) {

    if (argc < 3) {

        if (strcmp(argv[1], "--help") == 0) {
            printHelp();
            return 0;
        }

        std::cerr << "Usage: ./main <graph_file> k [OPTIONS]" << std::endl;
        return -1;
    }

    std :: cout << "\n-----------Loading and prepocessing-----------\n" << std::endl;

    // Load initial graph and create a NetworKit version of it.
    // Also creates groupCloseness object to compute f(S) later.
    Grover::Graph graph = loadGraph(argv[1]);
    NetworKit::Graph graphNk = convertToNetworKit(graph);

    const int k = std::stoi(argv[2]);
    if (k <= 0 || k > graph.get_num_verts()) {
        std::cerr << "Error: wrong k value. k must be in range 0 < k < n" << std::endl;
        return -1;
    }

    auto cfg = parseArguments(argc, argv);
    BooleanEncoder encoder(graph, cfg.encodings, cfg.verbose);
    encoder.preprocessGraph(graphNk, k, cfg.useHeuristic);

    GccmSolver gccmSolver;
    gccmSolver.solve(k, cfg.encodeExactAsAtMost, graphNk, encoder);

}