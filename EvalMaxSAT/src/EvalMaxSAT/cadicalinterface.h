// #pragma once


// #include "cadical/cadical.hpp"
// #include "cadical/internal.hpp"

// #include <cmath>
// #include <cassert>
// #include <vector>
// #include <set>
// #include <chrono>

// class CadicalTimeTerminator : public CaDiCaL::Terminator {
// protected:
//     int _timeout = 0;
//     std::chrono::high_resolution_clock::time_point _start;
// public:
//     CadicalTimeTerminator(int timeout) : _timeout(timeout), _start(std::chrono::high_resolution_clock::now()) {
//     }

//     bool terminate() override {
//         return duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - _start).count() >
//                _timeout;
//     }
// };

// class Solver_cadical {
//     CaDiCaL::Solver *solver;
//     std::vector<int> variables;

// public:

//     Solver_cadical(vector<set<uint32_t>> succ={},  vector<set<uint32_t>> pred={}, uint32_t g_size=0,
//                vector<uint32_t> initial_solution = {}) : solver(new CaDiCaL::Solver()) {
//         solver->set("factor", 0);
//     }

//     ~Solver_cadical() {
//         delete solver;
//     }

//     template<class T>
//     void exportClauses(T& to) const {
//         for (auto idx : solver->internal->vars) {
//             const int tmp = solver->internal->fixed (idx);
//             if (tmp) to.addClause({tmp < 0 ? -idx : idx});
//         }
//         for (const auto & c : solver->internal->clauses) {
//             if (!c->garbage) {
//                 std::vector<int> cl;
//                 for (const auto & lit : *c) {
//                     cl.push_back(lit);
//                 }
//                 to.addClause(cl);
//             }
//         }
//     }

//     bool getValue(int lit) {
//         return solver->val(lit) > 0;
//     }

//     std::vector<bool> getSolution() {
//         std::vector<bool> res;
//         res.reserve(variables.size()+1);
//         res.push_back(false);
//         for (auto cv : variables)
//             res.push_back(solver->val(cv) > 0);

//         return res;
//     }

//     unsigned int nVars() {
//         return variables.size();
//     }

//     int newVar(bool decisionVar=true) {
//         variables.push_back(solver->declare_one_more_variable());
//         return variables.back();
//     }

//     void addClause(const std::vector<int> &clause) {
//         for (int lit : clause) {
//             solver->add(lit);
//         }
//        solver->add(0);
//     }

//     void simplify() {
//         solver->simplify();
//     }

//     bool solve(const std::vector<bool>& solution) {
//         for(unsigned int i=1; i<solution.size(); i++) {
//             if(solution[i]) {
//                 solver->assume(i);
//             } else {
//                 solver->assume(-(int)i);
//             }
//         }

//         int result = solver->solve();

//         if( !( (result == 10) || (result == 20) ) ) {
//             return solve(solution);
//         }

//         return result == 10; // Sat
//     }

//     bool solve() {
//         int result = solver->solve();

//         //assert( (result == 10) || (result == 20) ); // Bug? Can happen sometimes...
//         if( !( (result == 10) || (result == 20) ) ) {
//             return solve();
//         }

//         return result == 10; // Sat
//     }

//     template<class T>
//     bool solve(const T &assumption) {

//         for (int lit : assumption) {
//             solver->assume(lit);
//         }

//         int result = solver->solve();

//         //assert( (result == 10) || (result == 20) ); // Bug? Can happen sometimes...
//         if( !( (result == 10) || (result == 20) ) ) {
//             return solve(assumption);
//         }
//         return result == 10; // Sat
//     }

//     template<class T>
//     bool solve(const T &assumption, const std::set<int> &forced) {
//         for (int lit : forced) {
//             solver->assume(lit);
//         }
//         for (int lit : assumption) {
//             if(forced.count(-lit) == 0) {
//                 solver->assume(lit);
//             }
//         }

//         int result = solver->solve();

//         //assert( (result == 10) || (result == 20) ); // Bug? Can happen sometimes...
//         if( !( (result == 10) || (result == 20) ) ) {
//             return solve(assumption, forced);
//         }

//         return result == 10; // Sat
//     }

//     template<class T>
//     int solveLimited(const T &assumption, int confBudget, int except=0) {
//         for (int lit : assumption) {
//             if (lit == except)
//                 continue;
//             solver->assume(lit);
//         }

//         solver->limit("conflicts", confBudget);

//         auto result = solver->solve();

//         if(result==10) { // Satisfiable
//             return 1;
//         }
//         if(result==20) { // Unsatisfiable
//             return 0;
//         }
//         if(result==0) { // Limit
//             return -1;
//         }

//         assert(false);
//         return 0;
//     }


//     template<class T>
//     int solveWithTimeout(const T &assumption, double timeout_sec, int except=0) {
//         solver->reset_assumptions();

//         for (int lit : assumption) {
//             if (lit == except)
//                 continue;
//             solver->assume(lit);
//         }

//         auto terminator = CadicalTimeTerminator(static_cast<int>(timeout_sec));
//         solver->connect_terminator(&terminator);
//         auto result = solver->solve();
//         solver->disconnect_terminator();

//         if(result==10) { // Satisfiable
//             return 1;
//         }
//         if(result==20) { // Unsatisfiable
//             return 0;
//         }
//         if(result==0) { // Limit
//             return -1;
//         }

//         assert(false);
//         return 0;
//     }


//     template<class T>
//     int solveWithTimeoutAndLimit(const T &assumption, double timeout_sec, int confBudget, int except=0) {
//         solver->reset_assumptions();

//         for (int lit : assumption) {
//             if (lit == except)
//                 continue;
//             solver->assume(lit);
//         }

//         solver->limit("conflicts", confBudget);
//         auto terminator = CadicalTimeTerminator(static_cast<int>(timeout_sec));
//         solver->connect_terminator(&terminator);
//         auto result = solver->solve();
//         solver->disconnect_terminator();

//         if(result==10) { // Satisfiable
//             return 1;
//         }
//         if(result==20) { // Unsatisfiable
//             return 0;
//         }
//         if(result==0) { // Limit
//             return -1;
//         }

//         assert(false);
//         return 0;
//     }

//     template<class T>
//     std::vector<int> getConflict(const T &assumptions) {
//         std::vector<int> conflicts;
//         for (int lit : assumptions) {
//             if (solver->failed(lit)) {
//                 conflicts.push_back(lit);
//             }
//         }
//         return conflicts;
//     }

//     bool propagate(const std::vector<int> &assum, std::vector<int> &result) {
//         for (auto ca : assum)
//             solver->assume(ca);
//         if (solver->propagate() == 20)
//             return false;

//         result.clear();
//         solver->implied(result);

//         return true;
//     }
// };


#pragma once

#include "cadical/cadical.hpp"
#include "cadical/internal.hpp"

#include <cmath>
#include <cassert>
#include <vector>
#include <set>
#include <chrono>

#include "MaLib/rand.h"

class CadicalTimeTerminator : public CaDiCaL::Terminator {
protected:
    int _timeout = 0;
    std::chrono::high_resolution_clock::time_point _start;
public:
    CadicalTimeTerminator(int timeout) : _timeout(timeout), _start(std::chrono::high_resolution_clock::now()) {
    }

    bool terminate() override {
        return duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - _start).count() >
               _timeout;
    }
};

class Solver_cadical {
    CaDiCaL::Solver *solver;
    CaDiCaL::ExternalPropagator *prop;
    std::vector<int> variables;

public:

    Solver_cadical(CaDiCaL::ExternalPropagator* (*propagator_factory)()) {
        solver = new CaDiCaL::Solver();
        solver->set("seed", MaLib::MonRand::get());
        solver->set("factor", 0);
        solver->set("exteagerreasons", 0);
        solver->set("exteagerrecalc", 0);

        if (propagator_factory != nullptr) {
            prop = propagator_factory();
            solver->connect_external_propagator(prop);
        }
    }

    ~Solver_cadical() {
        if (prop != nullptr) {
            solver->disconnect_external_propagator();
            delete prop;
        }
        delete solver;
    }

    template<class T>
    void exportClauses(T& to) const {
        for (auto idx : solver->internal->vars) {
            const int tmp = solver->internal->fixed (idx);
            if (tmp) to.addClause({tmp < 0 ? -idx : idx});
        }
        for (const auto & c : solver->internal->clauses) {
            if (!c->garbage) {
                std::vector<int> cl;
                for (const auto & lit : *c) {
                    cl.push_back(lit);
                }
                to.addClause(cl);
            }
        }
    }

    void print_statistics() {     
        uint64_t propagations = 0;
        propagations += solver->internal->stats.propagations.cover;
        propagations += solver->internal->stats.propagations.probe;
        propagations += solver->internal->stats.propagations.search;
        propagations += solver->internal->stats.propagations.transred;
        propagations += solver->internal->stats.propagations.vivify;
        // propagations += solver->internal->stats.propagations.backbone;
        // propagations += solver->internal->stats.propagations.instantiate;
        std::cout << "[solver_decisions] " << solver->internal->stats.decisions << "\n";
        std::cout << "[solver_conflicts] " << solver->internal->stats.conflicts << "\n";
        std::cout << "[solver_propagations] " << propagations << "\n";
        std::cout << "[solver_restarts] " << solver->internal->stats.restarts << "\n";
        std::cout << "[solver_backtracks] " << solver->internal->stats.backtracks << "\n";
    }

    bool getValue(int lit) {
        return solver->val(lit) > 0;
    }

    std::vector<bool> getSolution() {
        std::vector<bool> res;
        res.reserve(variables.size()+1);
        res.push_back(false);
        for (auto cv : variables)
            res.push_back(solver->val(cv) > 0);

        return res;
    }

    unsigned int nVars() {
        return variables.size();
    }

    int newVar(bool decisionVar=true) {
        // variables.push_back(solver->declare_one_more_variable());
        // return variables.back();

        int v = solver->declare_one_more_variable();
        variables.push_back(v);

        // std::cout << v << "\n";

        return v;
    }

    void addClause(const std::vector<int> &clause) {
        for (int lit : clause) {
            solver->add(lit);
        }
       solver->add(0);
    }

    void simplify() {
        solver->simplify();
    }

    bool solve(const std::vector<bool>& solution) {
        for(unsigned int i=1; i<solution.size(); i++) {
            if(solution[i]) {
                solver->assume(i);
            } else {
                solver->assume(-(int)i);
            }
        }

        int result = solver->solve();

        if( !( (result == 10) || (result == 20) ) ) {
            return solve(solution);
        }

        return result == 10; // Sat
    }

    bool solve() {
        int result = solver->solve();

        //assert( (result == 10) || (result == 20) ); // Bug? Can happen sometimes...
        if( !( (result == 10) || (result == 20) ) ) {
            return solve();
        }

        return result == 10; // Sat
    }

    template<class T>
    bool solve(const T &assumption) {

        for (int lit : assumption) {
            solver->assume(lit);
        }

        int result = solver->solve();

        //assert( (result == 10) || (result == 20) ); // Bug? Can happen sometimes...
        if( !( (result == 10) || (result == 20) ) ) {
            return solve(assumption);
        }
        return result == 10; // Sat
    }

    template<class T>
    bool solve(const T &assumption, const std::set<int> &forced) {
        for (int lit : forced) {
            solver->assume(lit);
        }
        for (int lit : assumption) {
            if(forced.count(-lit) == 0) {
                solver->assume(lit);
            }
        }

        int result = solver->solve();

        //assert( (result == 10) || (result == 20) ); // Bug? Can happen sometimes...
        if( !( (result == 10) || (result == 20) ) ) {
            return solve(assumption, forced);
        }

        return result == 10; // Sat
    }

    template<class T>
    int solveLimited(const T &assumption, int confBudget, int except=0) {
        for (int lit : assumption) {
            if (lit == except)
                continue;
            solver->assume(lit);
        }

        solver->limit("conflicts", confBudget);

        auto result = solver->solve();

        if(result==10) { // Satisfiable
            return 1;
        }
        if(result==20) { // Unsatisfiable
            return 0;
        }
        if(result==0) { // Limit
            return -1;
        }

        assert(false);
        return 0;
    }


    template<class T>
    int solveWithTimeout(const T &assumption, double timeout_sec, int except=0) {
        solver->reset_assumptions();

        for (int lit : assumption) {
            if (lit == except)
                continue;
            solver->assume(lit);
        }

        auto terminator = CadicalTimeTerminator(static_cast<int>(timeout_sec));
        solver->connect_terminator(&terminator);
        auto result = solver->solve();
        solver->disconnect_terminator();

        if(result==10) { // Satisfiable
            return 1;
        }
        if(result==20) { // Unsatisfiable
            return 0;
        }
        if(result==0) { // Limit
            return -1;
        }

        assert(false);
        return 0;
    }


    template<class T>
    int solveWithTimeoutAndLimit(const T &assumption, double timeout_sec, int confBudget, int except=0) {
        solver->reset_assumptions();

        for (int lit : assumption) {
            if (lit == except)
                continue;
            solver->assume(lit);
        }

        solver->limit("conflicts", confBudget);
        auto terminator = CadicalTimeTerminator(static_cast<int>(timeout_sec));
        solver->connect_terminator(&terminator);
        auto result = solver->solve();
        solver->disconnect_terminator();

        if(result==10) { // Satisfiable
            return 1;
        }
        if(result==20) { // Unsatisfiable
            return 0;
        }
        if(result==0) { // Limit
            return -1;
        }

        assert(false);
        return 0;
    }

    template<class T>
    std::vector<int> getConflict(const T &assumptions) {
        std::vector<int> conflicts;
        for (int lit : assumptions) {
            if (solver->failed(lit)) {
                conflicts.push_back(lit);
            }
        }
        return conflicts;
    }

    bool propagate(const std::vector<int> &assum, std::vector<int> &result) {
        for (auto ca : assum)
            solver->assume(ca);
        if (solver->propagate() == 20)
            return false;

        result.clear();
        solver->implied(result);

        return true;
    }
};
