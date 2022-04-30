#pragma once

#include <unordered_set>
#include <vector>

namespace sudoku
{

namespace types
{

using board_t = std::vector<std::vector<char>>;
using cell_t = std::unordered_set<char>;
using state_t = std::vector<std::vector<cell_t>>;
using remaining_t = size_t;
using percent_t = double;

}  // namespace types

}  // namespace sudoku
