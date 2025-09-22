// at top of solver.hpp (near other includes)
#if defined(__has_include)
#  if __has_include(<OsiClpSolverInterface.hpp>) && __has_include(<CbcModel.hpp>) && __has_include(<CoinPackedMatrix.hpp>)
#    define HAVE_CBC 1
#  endif
#endif

#if defined(HAVE_CBC)
#  include <OsiClpSolverInterface.hpp>
#  include <CbcModel.hpp>
#  include <CoinPackedMatrix.hpp>
#  include <CoinPackedVector.hpp>
#  include <cassert>
#  include <iostream>
#endif

class Solver {
public:
    enum class Method { Backtracking, MILP };

    Solver() = default;

    bool solve(Board &board, Method method = Method::Backtracking) {
        if (!board.isValid()) return false;
        if (method == Method::Backtracking) return solveBacktrack(board);
        return solveMilpCbc(board);
    }

private:
    // ... existing backtracking code here ...

#if defined(HAVE_CBC)
    // Helper: linear index for variable (r,c,v) where v in 1..9
    static inline int varIndex(int r, int c, int v) {
        return ((r*9 + c) * 9) + (v - 1);
    }

    bool solveMilpCbc(Board &b) {
        constexpr int R = 9, C = 9, V = 9;
        const int nvars = R * C * V; // 729

        // --- Prepare COO-like structure for constraints using CoinPackedMatrix
        // We'll accumulate each row (constraint) as a CoinPackedVector and append.

        std::vector<double> colLower(nvars, 0.0);
        std::vector<double> colUpper(nvars, 1.0);
        std::vector<double> objective(nvars, 0.0); // trivial objective

        // Fix variables for prefilled cells
        for (int r = 0; r < R; ++r) {
            for (int c = 0; c < C; ++c) {
                int val = b.get(r,c);
                if (val >= 1 && val <= 9) {
                    for (int v = 1; v <= V; ++v) {
                        int idx = varIndex(r,c,v);
                        if (v == val) {
                            colLower[idx] = 1.0;
                            colUpper[idx] = 1.0;
                        } else {
                            colLower[idx] = 0.0;
                            colUpper[idx] = 0.0;
                        }
                    }
                }
            }
        }

        // We'll build rows into a CoinPackedMatrix (row-major)
        CoinPackedMatrix matrix(false, 0, 0);

        std::vector<double> rowLower;
        std::vector<double> rowUpper;

        // 1) Cell constraints: sum_v x[r,c,v] == 1 for each cell
        for (int r = 0; r < R; ++r) {
            for (int c = 0; c < C; ++c) {
                CoinPackedVector row;
                for (int v = 1; v <= V; ++v) {
                    int idx = varIndex(r,c,v);
                    row.insert(idx, 1.0);
                }
                matrix.appendRow(row);
                rowLower.push_back(1.0);
                rowUpper.push_back(1.0);
            }
        }

        // 2) Row-value constraints: for each row r and value v, sum_c x[r,c,v] == 1
        for (int r = 0; r < R; ++r) {
            for (int v = 1; v <= V; ++v) {
                CoinPackedVector row;
                for (int c = 0; c < C; ++c) {
                    int idx = varIndex(r,c,v);
                    row.insert(idx, 1.0);
                }
                matrix.appendRow(row);
                rowLower.push_back(1.0);
                rowUpper.push_back(1.0);
            }
        }

        // 3) Col-value constraints: for each col c and value v, sum_r x[r,c,v] == 1
        for (int c = 0; c < C; ++c) {
            for (int v = 1; v <= V; ++v) {
                CoinPackedVector row;
                for (int r = 0; r < R; ++r) {
                    int idx = varIndex(r,c,v);
                    row.insert(idx, 1.0);
                }
                matrix.appendRow(row);
                rowLower.push_back(1.0);
                rowUpper.push_back(1.0);
            }
        }

        // 4) Block-value constraints: for each 3x3 block and value v, sum cells == 1
        for (int br = 0; br < 3; ++br) {
            for (int bc = 0; bc < 3; ++bc) {
                for (int v = 1; v <= V; ++v) {
                    CoinPackedVector row;
                    for (int i = 0; i < 3; ++i) for (int j = 0; j < 3; ++j) {
                        int r = br*3 + i;
                        int c = bc*3 + j;
                        int idx = varIndex(r,c,v);
                        row.insert(idx, 1.0);
                    }
                    matrix.appendRow(row);
                    rowLower.push_back(1.0);
                    rowUpper.push_back(1.0);
                }
            }
        }

        // Convert vectors to raw pointers for loadProblem
        int nrows = static_cast<int>(rowLower.size());
        // Note: CoinPackedMatrix::assignMight be used, but appendRow built it already.

        // --- Load problem into OSI solver (Clp)
        OsiClpSolverInterface solver;
        solver.loadProblem(matrix,
                           colLower.data(), colUpper.data(),
                           objective.data(),
                           rowLower.data(), rowUpper.data());

        // Mark all variables as integers (binary)
        for (int idx = 0; idx < nvars; ++idx) {
            solver.setInteger(idx);
        }

        // Optional: give a small MIP time limit (seconds)
        // solver.getModelPtr()->setMaximumSeconds(10); // not portable; skip

        // Build CbcModel from solver and solve
        CbcModel model(solver);
        model.setLogLevel(0); // suppress logging; set to 1 to debug
        model.branchAndBound();

        // Get solution (best integer solution)
        const double *best = model.bestSolution();
        if (!best) {
            // no integer solution found
            return false;
        }

        // Apply solution to board
        for (int r = 0; r < R; ++r) {
            for (int c = 0; c < C; ++c) {
                int found = 0;
                for (int v = 1; v <= V; ++v) {
                    int idx = varIndex(r,c,v);
                    double val = best[idx];
                    if (val > 0.5) {
                        found = v;
                        break;
                    }
                }
                if (found == 0) {
                    // inconsistent result (shouldn't happen)
                    return false;
                }
                b.set(r, c, found);
            }
        }
        return true;
    }
#else
    bool solveMilpCbc(Board &) {
        // CBC not available at compile time
        // If you want MILP support, link against CBC/Clp and ensure headers are available.
        return false;
    }
#endif

    // rest of private methods...
};

