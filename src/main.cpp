#include "board.hpp"
#include <iostream>

int main() {
    Board b("53..7....6..195... .98....6.8...6...34..8..3..17..2...6....28...419..5....8..79"); // 81 chars
    std::cout << b.toString();
    auto row2 = b.rowMap(2);        // modify a row directly
    auto col3 = b.colMap(3);        // modify a column directly
    auto block10 = b.blockMap(1,0); // middle-left 3x3 block
    // iterate empties
    for (auto [r,c] : b.emptyCells()) { /* ... */ }
}

