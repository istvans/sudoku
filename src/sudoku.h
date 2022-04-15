#pragma once

#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

namespace sudoku
{

class Solver
{
    using cell_t = std::unordered_set<char>;
    using state_t = std::vector<std::vector<cell_t>>;

    static const char maxValue = '9';
    static const size_t boxSize = 3;
    static const size_t numBoxes = 9;
    static const size_t numRows = 9;
    static const size_t numColumns = 9;
    static const size_t numElements = numRows * numColumns;

    struct Private;

    state_t state;
    size_t remaining;

public:
    class Exception: public std::runtime_error
    {
    public:
        explicit Exception(const std::string& msg): std::runtime_error(msg)
        {}
        virtual ~Exception() noexcept = 0;
    };

    class IAmStuck: public Exception
    {
    public:
        explicit IAmStuck(const std::string& msg): Exception(msg)
        {}
    };

    using board_t = std::vector<std::vector<char>>;

    void solve(board_t& board);

    void printState(bool simple = false) const;

    /** Parse a sudoku board from the command line parameters.
     *
     *  The program expects a single parameter in this format:
     *  '[[".",".","9","7","4","8",".",".","."],[...],...,[...]]'
     *
     *  If an error is encountered an error message is printed to stderr
     *  and the returned board is empty. */
    static board_t parseBoard(int argc, char* argv[]);
};

}  // namespace sudoku
