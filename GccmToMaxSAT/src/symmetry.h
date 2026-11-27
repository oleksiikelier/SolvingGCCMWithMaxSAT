//
// Created by alex on 22.11.26.
//

#ifndef GCCMTOMAXSAT_SYMMETRY_H
#define GCCMTOMAXSAT_SYMMETRY_H

#include "satsuma.h"
#include "utils.h"
#include "BooleanEncoder.h"

Solution<std::vector<int>> run_satsuma_maxsat(
    int n_vars,                  // max variable id in your WHOLE instance
    const std::vector<std::vector<int>>& hard,
    const std::vector<IntTuple>& softs,
    int break_depth);

void build_model_graph(
    cnf2wl& f,
    const std::vector<IntTuple>& softs,
    dejavu::static_graph& g,
    dejavu::coloring& col);

#endif //GCCMTOMAXSAT_SYMMETRY_H
