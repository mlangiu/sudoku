#include <QApplication>
#include "sudoku_widget.hpp"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    SudokuWidget w;
    Board b("53..7...."
            "6..195..."
            ".98....6."
            "8...6...3"
            "4..8.3..1"
            "7...2...6"
            ".6....28."
            "...419..5"
            "....8..79");
    w.setBoard(b);
    w.show();

    return app.exec();
}

