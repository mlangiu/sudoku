#pragma once
#include <Eigen/Dense>
#include <array>
#include <string>
#include <sstream>
#include <iomanip>
#include <stdexcept>

class Board {
public:
    // typedefs for convenience
    using Matrix9 = Eigen::Matrix<int, 9, 9, Eigen::RowMajor>;
    using Row = Eigen::Matrix<int, 1, 9, Eigen::RowMajor>;
    using RowMap = Eigen::Map<Row>;
    using ConstRowMap = Eigen::Map<const Row>;
    using Col = Eigen::Matrix<int, 9, 1>;
    using ColStride = Eigen::Stride<1, 9>;
    using ColMap = Eigen::Map<Col, 0, ColStride>;
    using ConstColMap = Eigen::Map<const Col, 0, ColStride>;
    using Block = Eigen::Matrix<int, 3, 3, Eigen::RowMajor>;
    using BlockStride = Eigen::Stride<9,1>;
    using BlockMap = Eigen::Map<Block, 0, BlockStride>;
    using ConstBlockMap = Eigen::Map<const Block, 0, BlockStride>;

    Board() { clear(); }

    /// Construct from a 81-char string ('.' or '0' for empty, '1'..'9' for digits).
    explicit Board(const std::string &flat81) {
        fromString(flat81);
    }

    void clear() {
        data_.setZero();
    }

    /// Set a cell (row, col) with value v in [0..9]
    void set(int row, int col, int v) {
        checkIdx(row, col);
        if (v < 0 || v > 9) throw std::out_of_range("value must be 0..9");
        data_(row, col) = v;
    }

    int get(int row, int col) const {
        checkIdx(row, col);
        return data_(row, col);
    }

    /// Return whether the board has no zeros
    bool isFilled() const {
        return (data_.array() != 0).all();
    }

    /// Validate current board for basic Sudoku constraints (no duplicates in row/col/block ignoring zeros)
    bool isValid() const {
        // rows
        for (int r = 0; r < 9; ++r) {
            std::array<bool,10> seen{}; // index 1..9
            for (int c = 0; c < 9; ++c) {
                int v = data_(r,c);
                if (v == 0) continue;
                if (v < 1 || v > 9) return false;
                if (seen[v]) return false;
                seen[v] = true;
            }
        }

        // cols
        for (int c = 0; c < 9; ++c) {
            std::array<bool,10> seen{};
            for (int r = 0; r < 9; ++r) {
                int v = data_(r,c);
                if (v == 0) continue;
                if (seen[v]) return false;
                seen[v] = true;
            }
        }

        // blocks
        for (int br = 0; br < 3; ++br) for (int bc = 0; bc < 3; ++bc) {
            std::array<bool,10> seen{};
            auto bm = blockMap(br, bc);
            for (int i = 0; i < 3; ++i) for (int j = 0; j < 3; ++j) {
                int v = bm(i,j);
                if (v == 0) continue;
                if (seen[v]) return false;
                seen[v] = true;
            }
        }

        return true;
    }

    /// Row view (non-const)
    RowMap rowMap(int r) {
        if (r < 0 || r >= 9) throw std::out_of_range("row");
        // RowMajor makes each row contiguous: start = &data_(r,0)
        return RowMap(&data_(r,0));
    }
    ConstRowMap rowMap(int r) const {
        if (r < 0 || r >= 9) throw std::out_of_range("row");
        return ConstRowMap(&data_(r,0));
    }

    /// Column view (non-const). Columns are not contiguous in RowMajor -> use stride
    ColMap colMap(int c) {
        if (c < 0 || c >= 9) throw std::out_of_range("col");
        // starting pointer is &data_(0,c)
        // for a 9x1 Column Map in RowMajor storage, elements are spaced by 9 (cols)
        // We provide a Map with explicit stride: outer=1, inner=9 (see note in header)
        return ColMap(&data_(0,c), ColStride());
    }
    ConstColMap colMap(int c) const {
        if (c < 0 || c >= 9) throw std::out_of_range("col");
        return ConstColMap(&data_(0,c), ColStride());
    }

    /// 3x3 block view with block indices br, bc in [0..2]
    BlockMap blockMap(int br, int bc) {
        if (br < 0 || br >= 3 || bc < 0 || bc >= 3) throw std::out_of_range("block index");
        int row0 = br * 3;
        int col0 = bc * 3;
        // start pointer
        return BlockMap(&data_(row0, col0), Eigen::Stride<9,1>());
    }
    ConstBlockMap blockMap(int br, int bc) const {
        if (br < 0 || br >= 3 || bc < 0 || bc >= 3) throw std::out_of_range("block index");
        int row0 = br * 3;
        int col0 = bc * 3;
        return ConstBlockMap(&data_(row0, col0), Eigen::Stride<9,1>());
    }

    /// Import from an 81-char string (row-major), '.' or '0' mean empty
    void fromString(const std::string &flat81) {
        if (flat81.size() != 81) throw std::invalid_argument("expected 81 chars");
        for (int i = 0; i < 81; ++i) {
            char ch = flat81[i];
            int v = 0;
            if (ch == '.' || ch == '0') v = 0;
            else if (ch >= '1' && ch <= '9') v = ch - '0';
            else throw std::invalid_argument("invalid char in input");
            int r = i / 9;
            int c = i % 9;
            data_(r,c) = v;
        }
    }

    /// Produce a human-readable ASCII board (simple)
    std::string toString() const {
        std::ostringstream oss;
        for (int r = 0; r < 9; ++r) {
            if (r % 3 == 0) oss << "+-------+-------+-------+\n";
            for (int c = 0; c < 9; ++c) {
                if (c % 3 == 0) oss << "| ";
                int v = data_(r,c);
                oss << (v == 0 ? '.' : char('0' + v)) << ' ';
            }
            oss << "|\n";
        }
        oss << "+-------+-------+-------+\n";
        return oss.str();
    }

    /// Simple utility to iterate empty cells
    std::vector<std::pair<int,int>> emptyCells() const {
        std::vector<std::pair<int,int>> out;
        out.reserve(81);
        for (int r = 0; r < 9; ++r) for (int c = 0; c < 9; ++c)
            if (data_(r,c) == 0) out.emplace_back(r,c);
        return out;
    }

    /// underlying matrix access (const)
    const Matrix9& matrix() const { return data_; }
    Matrix9& matrix() { return data_; }

private:
    Matrix9 data_;

    static void checkIdx(int r, int c) {
        if (r < 0 || r >= 9 || c < 0 || c >= 9)
            throw std::out_of_range("index out of range");
    }
};

