#pragma once
#include "board.hpp"
#include <array>
#include <vector>
#include <optional>
#include <algorithm>

class Solver {
public:
    enum class Method { Backtracking, MILP };

    Solver() = default;

    /// Solve in-place. Returns true on success (board filled and valid).
    bool solve(Board &board, Method method = Method::Backtracking) {
        if (!board.isValid()) return false;
        if (method == Method::Backtracking) return solveBacktrack(board);
        // MILP not implemented yet
        return false;
    }

private:
    // bitmask for candidates: bit 1<<v set means v is possible (v in 1..9)
    using Mask = unsigned short; // need at least 9 bits

    static inline Mask fullMask() { return (1u << 10) - 2u; } // bits 1..9 set (bit0 unused)

    // compute candidates for a single cell by checking row, col, block
    static Mask candidatesFor(const Board &b, int r, int c) {
        if (b.get(r,c) != 0) return 0;
        std::array<bool,10> seen{}; // 0..9, ignore index 0
        // row
        auto row = b.rowMap(r);
        for (int j = 0; j < 9; ++j) {
            int v = row(j);
            if (v != 0) seen[v] = true;
        }
        // col
        auto col = b.colMap(c);
        for (int i = 0; i < 9; ++i) {
            int v = col(i);
            if (v != 0) seen[v] = true;
        }
        // block
        int br = r / 3, bc = c / 3;
        auto block = b.blockMap(br, bc);
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j) {
                int v = block(i,j);
                if (v != 0) seen[v] = true;
            }

        Mask m = 0;
        for (int v = 1; v <= 9; ++v) if (!seen[v]) m |= (1u << v);
        return m;
    }

    // count bits in mask (only bits 1..9 expected)
    static int popcount9(Mask m) {
        // builtin popcount counts all bits; that's fine.
        return __builtin_popcount((unsigned) m);
    }

    // choose next empty cell using MRV heuristic, returning (r,c,mask)
    static std::optional<std::tuple<int,int,Mask>> chooseCellMRV(const Board &b) {
        int bestR = -1, bestC = -1;
        Mask bestMask = 0;
        int bestCount = 100;

        for (int r = 0; r < 9; ++r) {
            auto row = b.rowMap(r);
            for (int c = 0; c < 9; ++c) {
                if (row(c) != 0) continue;
                Mask m = candidatesFor(b, r, c);
                int cnt = popcount9(m);
                if (cnt == 0) return std::nullopt; // dead end
                if (cnt < bestCount) {
                    bestCount = cnt;
                    bestR = r; bestC = c; bestMask = m;
                    if (bestCount == 1) return std::make_optional(std::make_tuple(bestR,bestC,bestMask)); // perfect
                }
            }
        }
        if (bestR == -1) return {}; // no empty cells
        return std::make_optional(std::make_tuple(bestR,bestC,bestMask));
    }

    // main recursive backtracking
    bool solveBacktrack(Board &b) {
        auto choice = chooseCellMRV(b);
        if (!choice.has_value()) {
            // either solved (no empty) or dead end (no candidates -> optional nullopt handled earlier)
            // check for solved: if no empty cells remain -> success
            return b.isFilled();
        }
        auto [r, c, mask] = *choice;
        // iterate candidate values in ascending order
        for (int v = 1; v <= 9; ++v) {
            if ((mask & (1u << v)) == 0) continue;
            b.set(r, c, v);
            // quick validity check (we already ensured no duplicates by mask but keep defensive)
            if (b.isValid()) {
                if (solveBacktrack(b)) return true;
            }
            b.set(r, c, 0); // undo
        }
        return false;
    }
};

