#pragma once
#include <QWidget>
#include "board.hpp"

class SudokuWidget : public QWidget {
    Q_OBJECT
public:
    explicit SudokuWidget(QWidget *parent = nullptr);

    void setBoard(const Board &board);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    Board board_;
};

