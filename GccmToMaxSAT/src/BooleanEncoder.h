//
// Created by alex on 10.11.26.
//

#ifndef GCCMTOMAXSAT_BOOLEANENCODER_H
#define GCCMTOMAXSAT_BOOLEANENCODER_H

#include "graph.h"
#include "pblib/pb2cnf.h"


using Tuple = std::pair<int, int>;
using Dictionary = std::map<Tuple, int>;


class BooleanEncoder {

    public:

        bool verbose_;

        // Keeping track of the used variables in constraints helps to avoid confusion,
        // since encodings are adding auxiliary variables.
        int lastXVar = 1;
        int nextVar = 1;

        // Mapping of the x_v_i variables in the ILP
        // to the specific variable which will be used in the constraint.
        Dictionary variables;

        // Graph object with V, A and D sets as defined in paper.
        Grover::Graph graph_;

        // Constraints encoder.
        PB2CNF pb2_cnf;

        // Helps to aovid the usage of the unnecessary vertices.
        std::vector<bool> new_in_suff_set;

        std::vector<Tuple> softClauses;

        std::vector<std::vector<int32_t>> hardClauses;

        BooleanEncoder(const Grover::Graph &graph, bool verbose);

        ~BooleanEncoder()= default;

        // Maps variables x_v_i to variables inside the solver.
        void mapVariables();

        void preprocessGraph(NetworKit::Graph &nkGraph, int k, bool useHeuristic=false);

        void encodeObjectiveFunction();

        void encodeExactlyKConstraint(int k);

        void encodeExactlyOneConstraint();

        void encodeAtLeastOneConstraint();

};

#endif //GCCMTOMAXSAT_BOOLEANENCODER_H
