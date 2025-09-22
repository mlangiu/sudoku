#include "board.hpp"
#include "solver.hpp"
#include <iostream>

int main() {
    Board b(
        "...49.2.."
	"..6.1..47"
	"..4.5..9."
	"67.5.14.."
	"8...4.7.."
	"5498..3.6"
	"41.96.8.."
	"...1.59.4"
	".28.7..65"
    ); // 81 chars
    std::cout << b.toString() << '\n';
    auto row5 = b.rowMap(5);        // modify a row directly
    auto col5= b.colMap(5);        // modify a column directly
    auto block11 = b.blockMap(1,1); // middle-left 3x3 block
    
    std::cout << row5 << '\n' << col5 << '\n' << block11 << '\n';
    // iterate empties
    //    for (auto [r,c] : b.emptyCells()) { std::cout << r << ',' << c << '\n';}
    Solver s;
    bool ok = s.solve(b, Solver::Method::Backtracking);
    if (ok) {
        std::cout << "Solved:\n" << b.toString() << "\n";
    } else {
        std::cout << "No solution found.\n";
    }

}

