#pragma once

#include "solver.h"
#include "types.h"

namespace sudoku
{

namespace display
{

using state_t = types::state_t;

void printState(const state_t& state, bool useSimpleFormat);

}  // namespace display

}  // namespace sudoku
