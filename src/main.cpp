#include <cstring>
#include <iostream>

#include "sudoku.h"

int main(int argc, char* argv[])
{
    auto board = sudoku::Solver::parseBoard(argc, argv);

    int exit_code;
    if (board.empty()) {
        exit_code = 1;
    } else {
        sudoku::Solver solver;
        try {
            solver.solve(board);
            std::cout << "Solution:\n";
            exit_code = 0;
        } catch(const sudoku::Solver::IAmStuck& ex) {
            std::cout << "The solver stopped with this error: " << ex.what() << '\n';
            exit_code = 2;
        }
        solver.printState((argc >= 3) && (std::strcmp(argv[2], "simple") == 0));
    }

    return exit_code;
}
