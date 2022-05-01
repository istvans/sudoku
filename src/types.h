#pragma once

#include <unordered_set>
#include <vector>

namespace sudoku
{

namespace types
{

using board_t = std::vector<std::vector<char>>;
using cell_t = std::unordered_set<char>;
using cells_t = std::vector<std::vector<cell_t>>;
using remaining_t = size_t;
using percent_t = double;

struct state_t
{
    state_t();
    ~state_t() = default;
    state_t(const state_t&) = default;
    state_t& operator=(const state_t&) = default;
    state_t(state_t&&) = default;
    state_t& operator=(state_t&&) = default;

    void erase();

    cells_t cells = cells_t();
    remaining_t remaining = 0;
};

}  // namespace types

}  // namespace sudoku
