#include "sudoku.h"

#include <cassert>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace sudoku
{

Solver::Exception::~Exception() noexcept
{}

struct Solver::Private
{
    static void eraseState(Solver& self)
    {
        self.state.clear();
        self.remaining = 0;
    }

    static double percentUnknown(const Solver& self)
    {
        return static_cast<double>(self.remaining) / self.numElements * 100.0;
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

    static char getSingleCellValue(const cell_t& cell)
    {
        assert(cell.size() == 1);
        auto it = cell.begin();
        return *it;
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
            const auto conflictingValue = getSingleCellValue(src);
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

    struct Box_t
    {
        size_t rowIndex;
        size_t columnIndex;
        size_t size() const
        {
            return boxSize;
        }
    };

    static Box_t box_for_cell_index(size_t rowIndex, size_t columnIndex)
    {
        auto box = Box_t();
        auto boxRowIndex = (rowIndex / box.size());
        box.rowIndex = boxRowIndex * box.size();
        auto boxColumnIndex = (columnIndex / box.size());
        box.columnIndex = boxColumnIndex * box.size();
        return box;
    }

    /// FIXME I think this should really work very differently
    static void updateCellFromBox(Solver& self, cell_t& cell, size_t rowIndex, size_t columnIndex)
    {
        const auto box = box_for_cell_index(rowIndex, columnIndex);
        for (auto i = box.rowIndex; i < (box.rowIndex + box.size()); ++i) {
            for (auto j = box.columnIndex; j < (box.columnIndex + box.size()); ++j) {
                const auto& otherCell = self.state[i][j];
                eraseConflictInDestinationCell(otherCell, cell);
            }
        }
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
                    }
                }
            }
        }
        if (remainingBeforeUpdate == self.remaining) {
            throw IAmStuck("The last update was ineffective. Remaining: " +
                std::to_string(self.remaining) + " (" + std::to_string(percentUnknown(self)) + "%)");
        }
    }

    static void updateBoardFromState(std::vector<std::vector<char>>& board, const state_t& state)
    {
        for (auto i = 0; i < board.size(); ++i) {
            for (auto j = 0; j < board[i].size(); ++j) {
                board[i][j] = getSingleCellValue(state[i][j]);
            }
        }
    }

    struct BoxStrings
    {
        std::vector<std::string> lines;
        size_t longest = 0;
    };

    static BoxStrings generateBoxStrings(const Solver& self, bool simple)
    {
        auto boxStrings = BoxStrings();

        std::stringstream stream;
        size_t numProcessedElements = 0;
        auto& state = self.state;
        for (auto i = 0; i < state.size(); ++i) {
            for (auto j = 0; j < state[i].size(); ++j) {
                if (state[i][j].size() == 1) {
                    stream << Private::getSingleCellValue(state[i][j]);
                } else if (simple) {
                    stream << '.';
                } else {
                    stream << '@';
                    auto& possible_values = state[i][j];
                    for (auto it = possible_values.begin(); it != possible_values.end(); ++it) {
                        stream << *it;
                        if (std::next(it) != possible_values.end()) {
                            stream << ',';
                        }
                    }
                    stream << '@';
                }

                ++numProcessedElements;

                if (numProcessedElements != boxSize) {
                    stream << ' ';
                }

                if (numProcessedElements == boxSize) {
                    auto box_line = stream.str();
                    if (box_line.length() > boxStrings.longest) {
                        boxStrings.longest = box_line.length();
                    }
                    boxStrings.lines.emplace_back(box_line);
                    stream.str("");
                    stream.clear();
                    numProcessedElements = 0;
                }
            }
        }
        return boxStrings;
    }

    static void printHorizontalLine(size_t frame_width, const char* character = "═")
    {
        for (auto i = 0; i < frame_width; ++i) {
            std::cout << character;
        }
    }

    static void printHeader(size_t frame_width)
    {
        std::cout << "╔";
        printHorizontalLine(frame_width);
        std::cout << "╤";
        printHorizontalLine(frame_width);
        std::cout << "╤";
        printHorizontalLine(frame_width);
        std::cout << "╗\n";
    }

    static void printFooter(size_t frame_width)
    {
        std::cout << "╚";
        printHorizontalLine(frame_width);
        std::cout << "╧";
        printHorizontalLine(frame_width);
        std::cout << "╧";
        printHorizontalLine(frame_width);
        std::cout << "╝\n";
    }

    static void printMiddleHorizontalLine(size_t frame_width)
    {
        std::cout << "╟";
        printHorizontalLine(frame_width, "─");
        std::cout << "┼";
        printHorizontalLine(frame_width, "─");
        std::cout << "┼";
        printHorizontalLine(frame_width, "─");
        std::cout << "╢\n";
    }
};

void Solver::solve(board_t& board) {
    Private::createState(*this, board);

    std::cout << "Input:\n";
    this->printState(true);
    std::cout << "Unknown elements: " << this->remaining << " (" <<
        Private::percentUnknown(*this) << "%)\n";

    std::cout << "Working on a solution...\n";
    Private::solved(*this);
    while (!Private::solved(*this)) {
        Private::updateCells(*this);
    }

    std::cout << "A solution was found!\n";
    Private::updateBoardFromState(board, state);
}

void Solver::printState(bool simple) const
{
    const auto boxStrings = Private::generateBoxStrings(*this, simple);
    const auto frame_width = boxStrings.longest;

    Private::printHeader(frame_width);

    size_t num_processed_lines = 0;
    size_t row_index = 0;
    for (const auto& line: boxStrings.lines) {
        if (num_processed_lines == 0) {
            std::cout << "║";
        }

        std::cout << std::left << std::setw(frame_width) << line;
        ++num_processed_lines;

        if (num_processed_lines == boxSize) {
            std::cout << "║\n";
            num_processed_lines = 0;
            ++row_index;
            if (((row_index % boxSize) == 0) && (row_index != numRows)) {
                Private::printMiddleHorizontalLine(frame_width);
            }
        } else {
            std::cout << "│";
        }
    }

    Private::printFooter(frame_width);
}

Solver::board_t Solver::parseBoard(int argc, char* argv[])
{
    board_t board;
    if (argc > 1) {
        const auto input = std::string(argv[1]);

        int rowIndex = -1;
        enum State { StartNewRow, ExtractColumns, Abort };
        auto state = State::StartNewRow;
        for (auto ch: input) {
            switch (state) {
                case State::StartNewRow:
                    if (ch == '[') {
                        ++rowIndex;
                        board.emplace_back();
                        state = State::ExtractColumns;
                    }
                    break;
                case State::ExtractColumns:
                    if (ch == ']') {
                        if (board[rowIndex].size() != numColumns) {
                            std::cerr << "Expected " << numColumns << " columns in row " << (rowIndex + 1)
                                << ", got " << board[rowIndex].size() << '\n';
                            board.clear();
                            state = State::Abort;
                        } else {
                            state = State::StartNewRow;
                        }
                    } else if ((ch != '[') && (ch != ',') && (ch != '"') && (ch != '\'')) {
                        board[rowIndex].emplace_back(ch);
                    }
                    break;
                case State::Abort:
                    goto endloop;
            }
        }
endloop:

        if (!board.empty() && (board.size() != numRows)) {
            std::cerr << "Expected " << numRows << " rows, got " << board.size() << '\n';
            board.clear();
        }
    } else {
        const auto num_args = argc - 1;
        std::cerr << "Expected at least one argument, got " << num_args << '\n';
    }
    return board;
}

}  // namespace sudoku
