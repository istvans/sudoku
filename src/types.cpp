#include "types.h"

namespace sudoku
{

namespace types
{

state_t::state_t()
{}

void state_t::erase()
{
    this->cells.clear();
    this->remaining = 0;
}

}  // namespace types

}  // namespace sudoku
