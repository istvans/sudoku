#include <cassert>
#include <unordered_map>

#include "constants.h"
#include "display.h"
#include "solver.h"
#include "utils.h"


namespace sudoku
{

Solver::Exception::~Exception() noexcept
{}

struct Solver::Private
{
    static const auto maxValue = constants::maxValue;
    static const auto boxSize = constants::boxSize;
    static const auto numBoxes = constants::numBoxes;
    static const auto numRows = constants::numRows;
    static const auto numColumns = constants::numColumns;
    static const auto numElements = constants::numElements;

    static void eraseState(Solver& self)
    {
        self.state.clear();
        self.remaining = 0;
    }

    static percent_t unknownPercent(const Solver& self)
    {
        return static_cast<percent_t>(self.remaining) / numElements * 100.0;
    }

    static void createState(Solver& self, const board_t& board)
    {
        eraseState(self);

        for (auto i = 0; i < board.size(); ++i) {
            self.state.emplace_back();
            for (auto j = 0; j < board[i].size(); ++j) {
                self.state[i].emplace_back();
                if (board[i][j] == '.') {
                    ++self.remaining;
                    for (auto k = 0; k < 9; ++k) {
                        self.state[i][j].emplace('1' + k);
                    }
                } else {
                    self.state[i][j].emplace(board[i][j]);
                }
            }
        }
    }

    static bool solved(const Solver& self)
    {
        bool result = true;
        for (auto i = 0; i < self.state.size(); ++i) {
            for (auto j = 0; j < self.state[i].size(); ++j) {
                if (self.state[i][j].size() != 1) {
                    result = false;
                    break;
                }
            }
        }
        return result;
    }

    static void eraseConflictInDestinationCell(const cell_t& src, cell_t& dst)
    {
        if ((src != dst) && (src.size() == 1)) {
            const auto conflictingValue = utils::getSingleCellValue(src);
            dst.erase(conflictingValue);
        }
    }

    static void updateCellFromRow(Solver& self, cell_t& cell, size_t rowIndex)
    {
        const auto& row = self.state[rowIndex];
        for (const auto& otherCell: row) {
            eraseConflictInDestinationCell(otherCell, cell);
        }
    }

    static void updateCellFromColumn(Solver& self, cell_t& cell, size_t columnIndex)
    {
        for (auto rowIndex = 0; rowIndex < self.state.size(); ++rowIndex) {
            const auto& otherCell = self.state[rowIndex][columnIndex];
            eraseConflictInDestinationCell(otherCell, cell);
        }
    }

    class Box_t
    {
        static const size_t uninitialised = 42;
    public:
        size_t rowIndex;
        size_t columnIndex;
        bool needsUpdate;

        Box_t(): rowIndex(uninitialised), columnIndex(uninitialised), needsUpdate(false)
        {}

        static size_t size()
        {
            return boxSize;
        }

        size_t height() const
        {
            assert(rowIndex != uninitialised);
            return rowIndex + size();
        }

        size_t width() const
        {
            assert(columnIndex != uninitialised);
            return columnIndex + size();
        }

        bool isUninitialised() const
        {
            return (rowIndex == uninitialised) || (columnIndex == uninitialised);
        }

        size_t update(Solver& self)
        {
            size_t numFilledCells = 0;

            auto cellsOfValue = std::unordered_map<char, std::vector<std::pair<size_t, size_t>>>();
            for (char v = '1'; v <= maxValue; ++v)
            {
                cellsOfValue.emplace(v, decltype(cellsOfValue)::mapped_type());

                for (auto i = rowIndex; i < height(); ++i) {
                    for (auto j = columnIndex; j < width(); ++j) {
                        auto& boxCell = self.state[i][j];
                        if (boxCell.count(v) == 1) {
                            cellsOfValue[v].emplace_back(i, j);
                        }
                    }
                }

                if (cellsOfValue[v].size() == 1) {
                    auto [i, j] = cellsOfValue[v][0];
                    auto& cell = self.state[i][j];
                    if (cell.size() > 1) {
                        // only a single cell has v as a potential value, but that cell
                        // has still other values listed as potential values
                        // let's write `v` into that cell
                        cell.clear();
                        cell.emplace(v);
                        ++numFilledCells;
                    }
                }
            }

            return numFilledCells;
        }

        static size_t box_index_of_cell_index(size_t cellIndex)
        {
            return cellIndex / size();
        }

        static size_t box_index_of_cell(size_t rowIndex, size_t columnIndex)
        {
            const auto boxRowIndex = box_index_of_cell_index(rowIndex);
            const auto boxColIndex = box_index_of_cell_index(columnIndex);
            return boxRowIndex * size() + boxColIndex;
        }
    };

    using boxes_t = std::vector<Box_t>;
    static boxes_t boxes;

    static Box_t& box_for_cell_index(size_t rowIndex, size_t columnIndex)
    {
        const auto boxIndex = Box_t::box_index_of_cell(rowIndex, columnIndex);
        auto& box = boxes[boxIndex];
        if (box.isUninitialised()) {
            const auto boxRowIndex = Box_t::box_index_of_cell_index(rowIndex);
            box.rowIndex = boxRowIndex * box.size();
            const auto boxColumnIndex =  Box_t::box_index_of_cell_index(columnIndex);
            box.columnIndex = boxColumnIndex * box.size();
        }

        return box;
    }

    static void updateCellFromBox(Solver& self, cell_t& cell, size_t rowIndex, size_t columnIndex)
    {
        const auto& box = box_for_cell_index(rowIndex, columnIndex);
        for (auto i = box.rowIndex; i < box.height(); ++i) {
            for (auto j = box.columnIndex; j < box.width(); ++j) {
                const auto& otherCell = self.state[i][j];
                eraseConflictInDestinationCell(otherCell, cell);
            }
        }
    }

    static size_t updateMarkedBoxes(Solver& self)
    {
        size_t numFilledCells = 0;
        for (auto& box: boxes) {
            if (box.needsUpdate) {
                numFilledCells += box.update(self);
                box.needsUpdate = false;
            }
        }
        return numFilledCells;
    }

    static void updateCells(Solver& self)
    {
        const auto remainingBeforeUpdate = self.remaining;
        for (auto i = 0; i < self.state.size(); ++i) {
            for (auto j = 0; j < self.state[i].size(); ++j) {
                auto& cell = self.state[i][j];
                if (cell.size() != 1) {
                    updateCellFromRow(self, cell, i);
                    updateCellFromColumn(self, cell, j);
                    updateCellFromBox(self, cell, i, j);

                    if (cell.size() == 1) {
                        --self.remaining;
                    } else {
                        auto& box = box_for_cell_index(i, j);
                        box.needsUpdate = true;
                    }
                }
            }
        }

        self.remaining -= updateMarkedBoxes(self);

        if (remainingBeforeUpdate == self.remaining) {
            throw IAmStuck("The last update was ineffective. Remaining: " +
                std::to_string(self.remaining) + " (" + std::to_string(unknownPercent(self)) + "%)");
        }
    }

    static void updateBoardFromState(std::vector<std::vector<char>>& board, const state_t& state)
    {
        for (auto i = 0; i < board.size(); ++i) {
            for (auto j = 0; j < board[i].size(); ++j) {
                board[i][j] = utils::getSingleCellValue(state[i][j]);
            }
        }
    }
};

Solver::Private::boxes_t Solver::Private::boxes = Solver::Private::boxes_t(numBoxes);


Solver::Solver(board_t& board)
    : currentBoard(board)
{
    Private::createState(*this, this->currentBoard);
}

Solver::board_t Solver::solve()
{
    while (!Private::solved(*this)) {
        Private::updateCells(*this);
    }

    Private::updateBoardFromState(this->currentBoard, state);

    return this->currentBoard;
}

void Solver::printState(bool useSimpleFormat) const
{
    display::printState(state, useSimpleFormat);
}

Solver::remaining_t Solver::unknownCount() const
{
    return this->remaining;
}

Solver::percent_t Solver::unknownPercent() const
{
    return Private::unknownPercent(*this);
}

}  // namespace sudoku
