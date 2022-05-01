#include <cassert>
#include <unordered_map>

#include "constants.h"
#include "display.h"
#include "solver.h"
#include "utils.h"


namespace sudoku
{

using cell_t = types::cell_t;

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
        self.state.erase();
        self.forkStates.reset();
    }

    static percent_t unknownPercent(const Solver& self)
    {
        return static_cast<percent_t>(self.state.remaining) / numElements * 100.0;
    }

    static void createState(Solver& self, const board_t& board)
    {
        eraseState(self);

        for (auto i = 0; i < board.size(); ++i) {
            self.state.cells.emplace_back();
            for (auto j = 0; j < board[i].size(); ++j) {
                self.state.cells[i].emplace_back();
                if (board[i][j] == '.') {
                    ++self.state.remaining;
                    for (auto k = 0; k < 9; ++k) {
                        self.state.cells[i][j].emplace('1' + k);
                    }
                } else {
                    self.state.cells[i][j].emplace(board[i][j]);
                }
            }
        }
    }

    static bool solved(const Solver& self)
    {
        bool result = true;
        for (auto i = 0; i < self.state.cells.size(); ++i) {
            for (auto j = 0; j < self.state.cells[i].size(); ++j) {
                if (self.state.cells[i][j].size() != 1) {
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
        const auto& row = self.state.cells[rowIndex];
        for (const auto& otherCell: row) {
            eraseConflictInDestinationCell(otherCell, cell);
        }
    }

    static void updateCellFromColumn(Solver& self, cell_t& cell, size_t columnIndex)
    {
        for (auto rowIndex = 0; rowIndex < self.state.cells.size(); ++rowIndex) {
            const auto& otherCell = self.state.cells[rowIndex][columnIndex];
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
                        auto& boxCell = self.state.cells[i][j];
                        if (boxCell.count(v) == 1) {
                            cellsOfValue[v].emplace_back(i, j);
                        }
                    }
                }

                if (cellsOfValue[v].size() == 1) {
                    auto [i, j] = cellsOfValue[v][0];
                    auto& cell = self.state.cells[i][j];
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
                const auto& otherCell = self.state.cells[i][j];
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

    static bool updateCells(Solver& self)
    {
        const auto remainingBeforeUpdate = self.state.remaining;
        for (auto i = 0; i < self.state.cells.size(); ++i) {
            for (auto j = 0; j < self.state.cells[i].size(); ++j) {
                auto& cell = self.state.cells[i][j];
                if (cell.size() != 1) {
                    updateCellFromRow(self, cell, i);
                    updateCellFromColumn(self, cell, j);
                    updateCellFromBox(self, cell, i, j);

                    if (cell.size() == 1) {
                        --self.state.remaining;
                    } else {
                        auto& box = box_for_cell_index(i, j);
                        box.needsUpdate = true;
                    }
                }
            }
        }

        self.state.remaining -= updateMarkedBoxes(self);

        return remainingBeforeUpdate != self.state.remaining;
    }

    static void updateBoardFromState(std::vector<std::vector<char>>& board, const state_t& state)
    {
        for (auto i = 0; i < board.size(); ++i) {
            for (auto j = 0; j < board[i].size(); ++j) {
                board[i][j] = utils::getSingleCellValue(state.cells[i][j]);
            }
        }
    }
};

Solver::Private::boxes_t Solver::Private::boxes = Solver::Private::boxes_t(numBoxes);


Solver::Solver(board_t& board)
    : currentBoard(board)
{
    Private::createState(*this, currentBoard);
}

bool Solver::ForkStates::isExhausted()
{
    return exhausted;
}

bool Solver::ForkStates::isAlreadyForked()
{
    return alreadyForked;
}

bool Solver::ForkStates::findForkStates(const types::state_t& state)
{
    for (auto row = 0; row < state.cells.size(); ++row) {
        for (auto col = 0; col < state.cells[row].size(); ++col) {
            if (state.cells[row][col].size() == 2) {
                for (auto value: state.cells[row][col]) {
                    forks.push_back(state);  // yes, this is a copy
                    forks.back().cells[row][col].clear();
                    forks.back().cells[row][col].emplace(value);
                    --forks.back().remaining;
                }
            }
        }
    }

    alreadyForked = true;

    forksIter = forks.begin();

    return !forks.empty();
}

auto Solver::ForkStates::nextForkState()
{
    auto next = forksIter;
    ++forksIter;
    if (forksIter == forks.end()) {
        exhausted = true;
    }
    return *next;
}

void Solver::ForkStates::reset()
{
    forks.clear();
    exhausted = false;
    alreadyForked = false;
    forksIter = forks.end();
}

size_t Solver::ForkStates::count() const
{
    return forks.size();
}

Solver::board_t Solver::solve()
{
    while (!Private::solved(*this)) {
        const auto updated = Private::updateCells(*this);
        if (!updated) {
            if (forkStates.isExhausted()) {
                state = backupState;
                throw IAmStuck("I tried " + std::to_string(forkStates.count()) +
                    " forked states but still got stuck. Remaining: " +
                    std::to_string(state.remaining) + " (" + std::to_string(unknownPercent()) + "%)");
            } else if (forkStates.isAlreadyForked()) {
                state = forkStates.nextForkState();
            } else {
                if (forkStates.findForkStates(state)) {
                    backupState = state;
                    state = forkStates.nextForkState();
                } else {
                    throw IAmStuck("I can't find a suitable next step. Remaining: " +
                        std::to_string(state.remaining) + " (" + std::to_string(unknownPercent()) + "%)");
                }
            }
        }
    }

    Private::updateBoardFromState(currentBoard, state);

    return currentBoard;
}

void Solver::printState(bool useSimpleFormat) const
{
    display::printState(state, useSimpleFormat);
}

Solver::remaining_t Solver::unknownCount() const
{
    return state.remaining;
}

Solver::percent_t Solver::unknownPercent() const
{
    return Private::unknownPercent(*this);
}

}  // namespace sudoku
