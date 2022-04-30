#include "cli.h"

int main(int argc, char* argv[])
{
    return sudoku::CommandLine(argc, argv).run();
}
