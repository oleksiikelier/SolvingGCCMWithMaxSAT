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


static EncodingsTuple parseArguments(int argc, char *argv[]) {

    EncodingsTuple encodings;

    for (int i = 3; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "--amk") {

            std::string amkEncoder = argv[++i];

            if (amkEncoder == "card") {
                encodings.first = AMK_ENCODER::CARD;
            }

            else if (amkEncoder == "bdd") {
                encodings.first = AMK_ENCODER::BDD;
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
                encodings.second = AMO_ENCODER::BDD;
            }
            else if (amoEncoder == "kproduct") {
                encodings.second = AMO_ENCODER::KPRODUCT;
            }
            else if (amoEncoder == "bimmander") {
                encodings.second = AMO_ENCODER::BIMANDER;
            }
            else if (amoEncoder == "commander") {
                encodings.second = AMO_ENCODER::COMMANDER;
            }
            else if (amoEncoder == "binary") {
                encodings.second = AMO_ENCODER::BINARY;
            }
            else if (amoEncoder == "nested") {
                encodings.second = AMO_ENCODER::NESTED;
            }
            else if (amoEncoder == "pairwise") {
                encodings.second = AMO_ENCODER::PAIRWISE;
            }
            else {
                std::cerr << "Error: unknown encoding type for the AMO constraint: " << amoEncoder << std::endl;
                exit(-1);
            }

            std::cout << "AMO encoder: " << amoEncoder << std::endl;
        }
    }

    return encodings;
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

    auto chosenEncodings = parseArguments(argc, argv);
    BooleanEncoder encoder(graph, chosenEncodings, true);
    encoder.preprocessGraph(graphNk, k, false);

    GccmSolver gccmSolver;
    gccmSolver.solve(k, graphNk, encoder);

}