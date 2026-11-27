//
// Created by alex on 22.11.26.
//

#include <map>
#include "symmetry.h"

static int lit_vertex(int l) { return 2 * (std::abs(l) - 1) + (l < 0 ? 1 : 0); }

// Adapted code from satsuma library for the MaxSAT symmetry breaking
// Build the model graph from the formula
void build_model_graph(
    cnf2wl& f,
    const std::vector<IntTuple>& softs,
    dejavu::static_graph& g,
    dejavu::coloring& col
    )
{
    const int n = f.n_variables();
    const int m = f.n_clauses();

    // total soft weight per literal vertex
    std::vector<uint64_t> w(2 * n, 0);

    for (const auto& s : softs) {
        if (std::abs(s.first) < 1 || std::abs(s.first) > n)
            throw std::out_of_range("soft literal outside variable range");
        w[lit_vertex(s.first)] += s.second;
    }

    // one color per distinct weight, starting at 16 (0 = plain literal, 1 = clause)
    std::map<uint64_t, int> wcolor;

    for (uint64_t x : w) if (x != 0) wcolor.emplace(x, 0);
    int next_color = 16;
    for (auto &val: wcolor | std::views::values) val = next_color++;
    int unused_color = next_color;            // unique colors for completely unused variables

    // literal occurrence counts
    std::vector<int> uses(2 * n, 0);
    int total_lits = 0;
    for (int i = 0; i < m; ++i)
        for (int j = 0; j < f.clause_size(i); ++j) {
            ++uses[lit_vertex(f.literal_at_clause_pos(i, j))];
            ++total_lits;
        }

    g.initialize_graph(2 * n + m, n + total_lits);

    // literal vertices
    for (int v = 0; v < n; ++v) {
        const int vp = 2 * v, vn = 2 * v + 1;
        const bool unused = uses[vp] == 0 && uses[vn] == 0 && w[vp] == 0 && w[vn] == 0;
        auto color_of = [&](int vert) {
            if (w[vert] != 0) return wcolor[w[vert]];
            return unused ? unused_color++ : 0;
        };
        g.add_vertex(color_of(vp), uses[vp] + 1);
        g.add_vertex(color_of(vn), uses[vn] + 1);
    }
    // clause vertices
    for (int i = 0; i < m; ++i) g.add_vertex(1, f.clause_size(i));

    // edges: positive - negative literal of each variable
    for (int v = 0; v < n; ++v) g.add_edge(2 * v, 2 * v + 1);
    // edges: clause - literals (literal vertex < clause vertex always holds)
    for (int i = 0; i < m; ++i)
        for (int j = 0; j < f.clause_size(i); ++j)
            g.add_edge(lit_vertex(f.literal_at_clause_pos(i, j)), 2 * n + i);

    g.get_sgraph()->initialize_coloring(&col, g.get_coloring());
}

Solution<std::vector<int>> run_satsuma_maxsat(
    int n_vars,                  // max variable id in your WHOLE instance
    const std::vector<std::vector<int>>& hard,
    const std::vector<IntTuple>& softs,
    int break_depth
    )
{
    satsuma::preprocessor p;
    p.set_binary_clauses(true);
    // lex only: no fixing, no iterate, no CNF preprocessing (the graph must match the formula)
    p.set_optimize_generators(true);
    if (break_depth > 0) p.set_break_depth(break_depth);

    std::ostream null_log(nullptr);
    p.set_log_output(&null_log);
    satsuma::tracker track; track.h_silent = true;
    p.set_tracker(&track);

    cnf2wl f;
    int size = static_cast<int>(hard.size());
    f.reserve(n_vars - 1, size);
    for (auto c : hard) f.add_clause(c);

    dejavu::static_graph g;          // must outlive preprocess()
    dejavu::coloring     col;
    build_model_graph(f, softs, g, col);
    p.set_input_graph(g.get_sgraph());
    p.set_input_coloring(&col);

    Solution<std::vector<int>> res;
    p.set_result(&res);

    p.preprocess(f);

        Solution<std::vector<int>> out;
    out.nVars = res.nVars;
    out.container = std::move(res.container);

    return out;
}