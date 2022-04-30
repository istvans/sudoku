#include <cstring>
#include <iostream>

#include "cli.h"
#include "constants.h"


namespace sudoku
{

CommandLine::CommandLine(int argc, char* argv[])
{
    arguments.reserve(argc);
    for (auto i = 0; i < argc; ++i) {
        arguments.emplace_back(argv[i]);
    }
}

CommandLine::~CommandLine()
{}

Solver::board_t CommandLine::parseBoard(const Solver::arguments_t& args)
{
    Solver::board_t board;
    if (args.size() > 1) {
        const auto& input = args[1];

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
                        if (board[rowIndex].size() != constants::numColumns) {
                            std::cerr << "Expected " << constants::numColumns << " columns in row " << (rowIndex + 1)
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

        if (!board.empty() && (board.size() != constants::numRows)) {
            std::cerr << "Expected " << constants::numRows << " rows, got " << board.size() << '\n';
            board.clear();
        }
    } else {
        const auto num_args = args.size() - 1;
        std::cerr << "Expected at least one argument, got " << num_args << '\n';
    }
    return board;
}

int CommandLine::run()
{
    auto board = CommandLine::parseBoard(arguments);
    Solver solver(board);

    std::cout << "Input:\n";
    auto useSimpleFormat = true;
    solver.printState(useSimpleFormat);
    std::cout << "Unknown elements: " << solver.unknownCount() <<
        " (" << solver.unknownPercent() << "%)\n";

    int exitCode;
    if (board.empty()) {
        exitCode = 1;
    } else {
        sudoku::Solver solver(board);
        try {
            std::cout << "Working on a solution...\n";
            solver.solve();
            std::cout << "Solution:\n";
            exitCode = 0;
        } catch(const sudoku::Solver::IAmStuck& ex) {
            std::cout << "The solver stopped with this error: " << ex.what() << '\n';
            exitCode = 2;
        }
        useSimpleFormat = (arguments.size() >= 3) && (arguments[2] == "simple");
        solver.printState(useSimpleFormat);
    }

    return exitCode;
}

}  // namespace sudoku
