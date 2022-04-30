#include <cassert>

#include "utils.h"


namespace sudoku
{

namespace utils
{

char getSingleCellValue(const types::cell_t& cell)
{
    assert(cell.size() == 1);
    auto it = cell.begin();
    return *it;
}

}  // namespace utils

}  // namespace sudoku
