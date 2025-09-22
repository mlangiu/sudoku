#include "widget.hpp"
#include <QPainter>
#include <QFont>

SudokuWidget::SudokuWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(400, 400); // reasonable default size
}

void SudokuWidget::setBoard(const Board &board) {
    board_ = board;
    update(); // trigger repaint
}

void SudokuWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.fillRect(rect(), Qt::white);

    const int N = 9;
    int cellW = width() / N;
    int cellH = height() / N;

    QFont f = p.font();
    f.setPointSizeF(std::min(cellW, cellH) * 0.5);
    p.setFont(f);
    p.setPen(Qt::black);

    // draw grid + numbers
    for (int r = 0; r < N; ++r) {
        for (int c = 0; c < N; ++c) {
            QRect cell(c*cellW, r*cellH, cellW, cellH);
            p.drawRect(cell);

            int v = board_.get(r, c);
            if (v > 0) {
                p.drawText(cell, Qt::AlignCenter, QString::number(v));
            }
        }
    }

    // draw thicker lines for 3x3 blocks
    QPen thickPen(Qt::black, 3);
    p.setPen(thickPen);
    for (int i = 0; i <= N; i += 3) {
        // vertical
        p.drawLine(i*cellW, 0, i*cellW, height());
        // horizontal
        p.drawLine(0, i*cellH, width(), i*cellH);
    }
}

