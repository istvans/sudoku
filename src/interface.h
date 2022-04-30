#pragma once

namespace sudoku
{

struct Interface
{
    virtual ~Interface() = 0;
    virtual int run() = 0;
};

}  // namespace sudoku
