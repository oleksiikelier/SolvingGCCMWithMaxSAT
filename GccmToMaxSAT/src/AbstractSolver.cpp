//
// Created by alex on 25.11.26.
//
#include "AbstractSolver.h"

#include "networkit/distance/Eccentricity.hpp"

void AbstractSolver::precomputeEccs(NetworKit::Graph const &originalGraph, BooleanEncoder const &encoder) {

    TimePoint eccTime = Clock::now();

    // Original size of the graph should be used, since we can hit
    // a vertex which can have a potentially bigger index in the originalgraph  than in the reduced one
    cachedEccs.assign(encoder.graph_.n, 0);

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

void AbstractSolver::printLoggingTable() {
    auto [rows, cols] = loggingTable.dimensions();

    for (size_t i = 0; i < cols; ++i) {

        loggingTable[0][i].format()
        .font_color(tabulate::Color::yellow)
        .font_align(tabulate::FontAlign::center)
        .font_style({tabulate::FontStyle::bold});

        for (size_t j = 1; j < rows; ++j) {
            loggingTable[j][i].format()
            .font_align(tabulate::FontAlign::center);
        }

    }

    std::cout << "\n" << loggingTable << "\n" << std::endl;
}

void AbstractSolver::prepareLoggingTalbe() {
    loggingTable.add_row(
    {
        "i",
        "f(S)",
        "Score",
        "EO",
        "EK",
        "ALO",
        "# Hard",
        "# Soft",
        "Encoding time (s)",
        "Solving time (s)"
    });
}
