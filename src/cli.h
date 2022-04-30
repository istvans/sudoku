#pragma once

#include "interface.h"
#include "solver.h"


namespace sudoku
{

class CommandLine: public Interface
{
    Solver::arguments_t arguments;
public:
    CommandLine(int argc, char* argv[]);
    ~CommandLine();

    /** Parse a sudoku board from the command line parameters.
     *
     *  The program expects a single parameter in this format:
     *  '[[".",".","9","7","4","8",".",".","."],[...],...,[...]]'
     *
     *  If an error is encountered an error message is printed to stderr
     *  and the returned board is empty. */
    static Solver::board_t parseBoard(const Solver::arguments_t& args);

    int run() override;
};

}  // namespace sudoku
