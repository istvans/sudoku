#pragma once

namespace sudoku
{

struct Interface
{
    virtual ~Interface() noexcept = 0;
    virtual int run() = 0;
};

}  // namespace sudoku
