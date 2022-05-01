#pragma once

#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

#include "types.h"


namespace sudoku
{

class Solver
{
    using state_t = types::state_t;

    class ForkStates
    {
        using forks_t = std::vector<types::state_t>;
        forks_t forks;
        bool exhausted = false;
        bool alreadyForked = false;
        forks_t::iterator forksIter = forks.end();
    public:
        bool isExhausted();
        bool isAlreadyForked();
        bool findForkStates(const types::state_t& state);
        auto nextForkState();
        void reset();
        size_t count() const;
    };

    types::board_t currentBoard;
    ForkStates forkStates;
    state_t backupState;
    state_t state;
    struct Private;

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

    using arguments_t = std::vector<std::string>;
    using board_t = types::board_t;
    using remaining_t = types::remaining_t;
    using percent_t = types::percent_t;

    Solver(board_t& board);

    board_t solve();

    void printState(bool useSimpleFormat) const;
    remaining_t unknownCount() const;
    percent_t unknownPercent() const;
};

}  // namespace sudoku
