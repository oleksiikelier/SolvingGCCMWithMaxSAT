//
// Created by alex on 10.11.26.
//

#ifndef GCCMTOMAXSAT_BOOLEANENCODER_H
#define GCCMTOMAXSAT_BOOLEANENCODER_H

#include "graph.h"
#include "pblib/pb2cnf.h"


using IntTuple = std::pair<int, int>;
using EncodingsTuple = std::pair<AMK_ENCODER::PB2CNF_AMK_Encoder, AMO_ENCODER::PB2CNF_AMO_Encoder>;
using Dictionary = std::map<IntTuple, int>;


class BooleanEncoder {

    public:

        size_t objectiveSize = 0;
        size_t firstConstraintSize = 0;
        size_t secondConstraintSize = 0;
        size_t thirdConstraintSize = 0;

        bool verbose_;

        std::string graphName_;

        // Mapping of the x_v_i variables in the ILP
        // to the specific variable which will be used in the constraint.
        Dictionary variables;

        // Graph object with V, A and D sets as defined in paper.
        Grover::Graph graph_;

        std::unique_ptr<PB2CNF> pb2cnf;

        //std::unique_ptr<PBConfigClass>
        PBConfig pbLibConfig;

        // Helps to aovid the usage of the unnecessary vertices.
        std::vector<bool> new_in_suff_set;

        std::vector<IntTuple> softClauses;

        std::vector<std::vector<int>> hardClauses;

        BooleanEncoder(const Grover::Graph &graph, EncodingsTuple const &encodings, bool verbose, const std::string &graphName);

        ~BooleanEncoder()= default;

        // Maps variables x_v_i to variables inside the solver.
        void mapVariables();

        void resetEncoder();

        void preprocessGraph(NetworKit::Graph &nkGraph, int k, bool useHeuristic=false);

        // Returns variables of the objective function
        std::vector<int32_t> getVariables();

        void encode(int k, bool encodeExactAsAtMost);

        void encodeObjectiveFunction();

        void encodeExactlyKConstraint(int k);

        void encodeExactlyOneConstraint();

        void encodeAtLeastOneConstraint();

        void encodeAtMostKConstraint(int k);

        void saveHardClauses(const std::string &fileName);

        [[nodiscard]] int getNumberOfUsedVarialbes() const;

    private:
        // Keeping track of the used variables in constraints helps to avoid confusion,
        // since encodings are adding auxiliary variables.
        int lastXVar = 1;
        int nextVar = 1;

};

#endif //GCCMTOMAXSAT_BOOLEANENCODER_H
