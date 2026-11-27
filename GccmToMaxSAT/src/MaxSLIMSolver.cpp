//
// Created by alex on 25.11.26.
//

#include "MaxSLIMSolver.h"

#include "networkit/centrality/GroupCloseness.hpp"

MaxSlimSolver::MaxSlimSolver(Configuration const &cfg) {
    this->binary_ = cfg.maxSlimBinaryPath;
    this->timeout_ = cfg.timeout;
    this->maxIterations = cfg.maxNumberOfIterations;
    this->binary_ = cfg.maxSlimBinaryPath;
    this->saveWcnf_ = cfg.saveWcnf;
    this->encodeExactAsAtMost_ = cfg.encodeExactAsAtMost;

    if (!this->saveWcnf_) {
        prepareTempFile();
    }

    prepareLoggingTalbe();

}

void MaxSlimSolver::cleanTempFile() const {
    fclose(this->f);
    unlink(filename);
}

void MaxSlimSolver::prepareTempFile() {
    unlink(filename);
    int fileDescr = mkstemp(filename);

    if (fileDescr < 0) {
        std::cerr << RED "Error: cannot create file." RESET << std::endl;
        exit(-1);
    }

    f = fdopen(fileDescr, "w+");

    if (!f) {
        std::cerr << RED "Error: fdopen can't associate a stream with a file descriptor." RESET << std::endl;
    }
}

std::vector<uint64_t> MaxSlimSolver::solve(int k, NetworKit::Graph &originalGraph, BooleanEncoder &encoder) {
    std::vector<uint64_t> solution;
    int iteration = 1;

    // Needed for f(S) computation
    NetworKit::GroupCloseness gc(originalGraph);

    // Instead of computing it in every iteration, precompute once and use them later.
    // Shouldn't cause any problems since the graph itself does not change during the loop.
    this->precomputeEccs(originalGraph, encoder);

    std :: cout << "----------- Starting the main loop -----------\n" << std::endl;

    while (iteration < this->maxIterations) {

        TimePoint time = Clock::now();
        encoder.encode(k, encodeExactAsAtMost_);
        double encodingTime = elapsedSeconds(time, Clock::now());

        time = Clock::now();
        auto maxSlimSol = runMaxSlim(encoder, iteration);
        double solvingTime = elapsedSeconds(time, Clock::now());

        if (!maxSlimSol) {
            std::cerr << RED "\nError: runMaxSlim returned nullpointer.\n" RESET << std::endl;
            cleanTempFile();
            exit(-1);
        }

        solution = getSolution(encoder, maxSlimSol->container);

        std::vector<Grover::NodeID> nodesToAdvance = getNodesToAdvance(encoder, maxSlimSol->container);
        auto farness = static_cast<uint64_t>(gc.computeFarness(solution));

        loggingTable.add_row({
            std::to_string(iteration),
            std::to_string(farness),
            std::to_string(maxSlimSol->score),
            std::to_string(encoder.firstConstraintSize),
            std::to_string(encoder.secondConstraintSize),
            std::to_string(encoder.thirdConstraintSize),
            std::to_string(encoder.hardClauses.size()),
            std::to_string(encoder.softClauses.size()),
            std::to_string(encodingTime),
            std::to_string(solvingTime)
        });

        if (nodesToAdvance.empty() || maxSlimSol->score == farness) {
            std::cout << "Solution is optimal." << std::endl;

            cleanTempFile();
            return solution;
        }

        iteration++;

        std::cout
        << "Current solution is not optimal. Size of the nodesToAdvance: "
        << nodesToAdvance.size()
        << "\n\n--------------------------------------------------------------------------------------------\n"
        << std::endl;

        encoder.graph_.advanceAllBFS(nodesToAdvance);
        encoder.resetEncoder();
        nodesToAdvance.clear();
    }

    std::cout
    << YELLOW "\n(Warning):" RESET
    "Iteration limit is exceeded. Best found solution is not optimal. Consider increasing allowed number of iterations."
    << std::endl;

    cleanTempFile();

    return solution;
}

std::vector<Grover::NodeID> MaxSlimSolver::getNodesToAdvance(BooleanEncoder &encoder, std::vector<bool> &currentAssignment) {

    std::vector<Grover::NodeID> nodesToAdvance;

    for (int v = 0; v < encoder.graph_.r_n; ++v) {

        auto dv = encoder.graph_.d(v);
        auto lit = encoder.variables.at({v, dv});
        auto ecc = cachedEccs[encoder.graph_.translate[v]];

        // Early exit condintion: x_v_dv = 1 && ecc > d(v)
        if (currentAssignment[lit - 1] && ecc > dv) {
            nodesToAdvance.push_back(v);
        }
    }

    return nodesToAdvance;
}

std::vector<uint64_t> MaxSlimSolver::getSolution(BooleanEncoder &encoder, std::vector<bool> &currentAssignment) {
    std::vector<uint64_t> solution;

    for (const auto &mappedVariable: encoder.variables) {

        // mappedVariable maps variables x_v_i to a unique variable inside the solver.
        //
        // Find all x_v_0 variables which value inside the solver is equal to 1.
        const int v = mappedVariable.first.first;
        const int i = mappedVariable.first.second;

        if (i == 0 && currentAssignment[mappedVariable.second - 1]) {
            solution.push_back(encoder.graph_.translate[v]);
        }
    }

    return solution;
}

std::optional<Solution<bool>> MaxSlimSolver::runMaxSlim(BooleanEncoder const &encoder,int iteration) const {


    TimePoint savingWcnfStart = Clock::now();
    std::string file = filename;

    if (this->saveWcnf_) {
       file = saveWcnf(encoder, iteration);
    }
    else {
        saveWcnf(encoder);
    }

    std::cout
    << CYAN "(saveWcnf)" RESET " Time: "
    << elapsedSeconds(savingWcnfStart, Clock::now())
    << "(s)\n" << std::endl;

    //------------------------------------------------------------------------------------------------------------

    // create MaxSLIM cli command
    //
    // ./path_to_binary/maxslim -timeout=N file

    std::string cmd = this->binary_ + " -timeout=" + std::to_string(this->timeout_) + " " + file;
    std:: cout << CYAN "(MaxSLIM)" RESET " Running: " << cmd << "\n" << std::endl;

    //------------------------------------------------------------------------------------------------------------

    // Run MaxSLIM as a subprocess
    TimePoint solvingStart = Clock::now();
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        return std::nullopt;
    }

    //------------------------------------------------------------------------------------------------------------

    std::string lastO, lastV;
    char buf[1 << 16];
    std::string line;

    while (fgets(buf, sizeof buf, pipe)) {
        line = buf;
        if (line.rfind("o ", 0) == 0) lastO = line;
        else if (line.rfind("v ", 0) == 0) lastV = line;
    }

    int status = pclose(pipe);

    // No solution
    if (lastO.empty() || lastV.empty()) {
        return std::nullopt;
    }

    //------------------------------------------------------------------------------------------------------------

    // Put bitstring into boolean vector
    Solution<bool> solution;
    solution.score = static_cast<int>(std::stoull(lastO.substr(2)));

    // char[i] is the value of variable i + 1
    for (size_t i = 2; i < lastV.size(); ++i)
        if (lastV[i] == '0' || lastV[i] == '1') solution.container.push_back(lastV[i] == '1');

    (void)status;   // inspect WEXITSTATUS(status) if you want to log failures

    std::cout
    << CYAN "(MaxSLIM solving + parsing)" RESET " Time: "
    << elapsedSeconds(solvingStart, Clock::now()) << "(s)\n"
    << std::endl;

    return solution;

}

void MaxSlimSolver::saveWcnf(BooleanEncoder const &encoder) const {

    // Clear file from the output of the previous iteration
    std::fflush(this->f);
    if (ftruncate(fileno(this->f), 0) != 0) {
        std::cerr << RED "Error: ftruncate" RESET << std::endl;
    }
    rewind(f);

    for (const auto & litWeight : encoder.softClauses) {
        const std::string line = std::to_string(litWeight.second) + " " + std::to_string(litWeight.first) + " 0\n";
        std::fputs(line.data(), this->f);
    }

    for (const auto & clause: encoder.hardClauses) {

        std::fputs("h ", this->f);
        for (const auto & lit : clause) {
            std::fputs((std::to_string(lit) + " ").c_str(), this->f);
        }
        std::fputs("0\n", this->f);
    }

    std::fflush(this->f);
}

std::string MaxSlimSolver::saveWcnf(BooleanEncoder const &encoder, int iteration) {
    const std::string file =  WCNF_FOLDER + encoder.graphName_ + "_iteration_" + std::to_string(iteration) +  ".wcnf";

    std::ofstream out(file);

    if (!out.is_open()) {
        std::cerr << "\nCannot open file " << file << std::endl;
        exit(-1);
    }

    for (const auto & litWeight : encoder.softClauses) {

        out << litWeight.second << " " << litWeight.first << " 0\n";
    }

    for (const auto & clause: encoder.hardClauses) {
        out << "h ";
        for (const auto & lit : clause) {
            out << lit << " ";
        }
        out << "0\n";
    }
    out.close();

    return file;
}