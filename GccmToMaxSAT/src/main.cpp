#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "graph.h"

#include "EvalMaxSATSolver.h"
#include "MaxSLIMSolver.h"
#include "BooleanEncoder.h"
#include "utils.h"

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
        else if (arg == "--symbreak") {
            cfg.useSymBreaking = true;
        }
        else if (arg == "--slim") {
            cfg.useMaxSLIM = true;
        }
        else if (arg == "--timeout") {
            cfg.timeout = std::stoi(argv[++i]);
            if (cfg.timeout <= 0) {
                std::cerr << RED "Error: timeout for MaxSLIM needs to be a positive integer." RESET << std::endl;
                exit(-1);
            }
        }
        else if (arg == "--bin") {
            cfg.maxSlimBinaryPath = argv[++i];
        }
        else if (arg == "--maxIters") {
            cfg.maxNumberOfIterations = std::stoi(argv[++i]);
            if (cfg.maxNumberOfIterations <= 0) {
                std::cerr << RED "Error: max number of iterations should be a positive integer." RESET << std::endl;
                exit(-1);
            }
        }
        else if (arg == "--savewcnf") {
            cfg.saveWcnf = true;
        }
    }

    return cfg;
}

static void printHelp() {
    std::cout << "\n\nUsage:\n";
    std::cout << "./cmake-build-debug/GccmToMaxSAT/GccmToMaxSAT <graph_file> k [OPTIONS]\n\n";

    std::cout << "General options:\n";
    std::cout << "--amk <encoding>      Use <encoding> to encode the AT MOST K constraint.\n";
    std::cout << "                      <encoding> can take following values: card, bdd.\n\n";
    std::cout << "--amo <encoding>      Use <encoding> to encode the AT MOST ONE constraint.\n";
    std::cout << "                      <encoding> can take following values: bdd, bimmander,\n";
    std::cout << "                      commander, binary, nested, pairwise, kproduct\n\n";
    std::cout << "--verbose             Print additional information during the encoding process.\n\n";
    std::cout << "--eaam                Encode =k constraint as <= k.\n\n";
    std::cout << "--heuristic           Instead of setting d(v) = 2, use a heuristic solution as a starting point.\n\n";
    std::cout << "--maxIters            Maximum number of iterations that solver is allowed to run. If optimal solution wasn't found\n\n";

    std::cout << "EvalMaxSAT specific options:\n";
    std::cout << "--symbreak            Uses satsuma to preprocess the formula and break symmetries.\n\n";

    std::cout << "MaxSLIM specific options:\n";
    std::cout << "--slim                Use MaxSLIM instead of EvalMaxSAT. Requires a compiled MaxSLIM binary.\n";
    std::cout << "                      Path to the binary needs to be adjusted in GccmToMaxSAT/src/utils.h MAXSLIM_BIN.\n\n";
    std::cout << "--bin                 If MaxSLIM is used it's mandatory to provide a timeout, since it's an anytime solver.\n\n";
    std::cout << "                      solver returns the last found solution\n\n";
    std::cout << "--savewcnf            Saves wcnf formula in each iteration in GccmSolver/src/wcnfs.\n";
    std::cout << "                      If not specified, then solver uses temporary file. \n\n";
    std::cout << "                      Currently only works with MaxSLIMSolver. \n\n";

}

static void printSolution(const std::vector<uint64_t> &solution) {
    std::cout << "S = { ";
    for (const auto &var: solution) {
        std :: cout << var << " ";
    }
    std::cout << "}" << std::endl;
}

int main(int argc, char *argv[]) {

    if (strcmp(argv[1], "--help") == 0) {
        printHelp();
        return 0;
    }

    if (argc < 3) {
        std::cerr << "Usage: ./main <graph_file> k [OPTIONS]" << std::endl;
        return -1;
    }

    std :: cout << "\n----------- Loading and prepocessing ---------\n" << std::endl;

    // Load initial graph and create a NetworKit version of it.
    Grover::Graph graph = loadGraph(GRAPHS_FOLDER + argv[1]);
    NetworKit::Graph graphNk = convertToNetworKit(graph);

    const int k = std::stoi(argv[2]);
    if (k <= 0 || k > graph.get_num_verts()) {
        std::cerr << "Error: wrong k value. k must be in range 0 < k < n" << std::endl;
        return -1;
    }

    auto cfg = parseArguments(argc, argv);
    BooleanEncoder encoder(graph, cfg.encodings, cfg.verbose, argv[1]);
    encoder.preprocessGraph(graphNk, k, cfg.useHeuristic);


    TimePoint totalRuntime = Clock::now();

    if (cfg.useMaxSLIM) {
        MaxSlimSolver solver(cfg);
        auto solution = solver.solve(k, graphNk, encoder);
        solver.printLoggingTable();
        printSolution(solution);
    }
    else {
        EvalMaxSATSolver solver(cfg);
        auto solution = solver.solve(k, graphNk, encoder);
        solver.printLoggingTable();
        printSolution(solution);
    }


    std::cout << "\nTotal time: " << elapsedSeconds(totalRuntime, Clock::now()) << "(s)" << std::endl;

    return 0;
}