#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "constants.h"
#include "display.h"
#include "utils.h"


namespace sudoku
{

namespace display
{

struct BoxStrings
{
    std::vector<std::string> lines;
    size_t longest = 0;
};

static void printHorizontalLine(size_t frame_width, const char* character = "═")
{
    for (auto i = 0; i < frame_width; ++i) {
        std::cout << character;
    }
}

static void printHeader(size_t frame_width)
{
    std::cout << "╔";
    printHorizontalLine(frame_width);
    std::cout << "╤";
    printHorizontalLine(frame_width);
    std::cout << "╤";
    printHorizontalLine(frame_width);
    std::cout << "╗\n";
}

static void printFooter(size_t frame_width)
{
    std::cout << "╚";
    printHorizontalLine(frame_width);
    std::cout << "╧";
    printHorizontalLine(frame_width);
    std::cout << "╧";
    printHorizontalLine(frame_width);
    std::cout << "╝\n";
}

static void printMiddleHorizontalLine(size_t frame_width)
{
    std::cout << "╟";
    printHorizontalLine(frame_width, "─");
    std::cout << "┼";
    printHorizontalLine(frame_width, "─");
    std::cout << "┼";
    printHorizontalLine(frame_width, "─");
    std::cout << "╢\n";
}

static BoxStrings generateBoxStrings(const state_t& state, bool useSimpleFormat)
{
    auto boxStrings = BoxStrings();

    std::stringstream stream;
    size_t numProcessedElements = 0;
    for (auto i = 0; i < state.size(); ++i) {
        for (auto j = 0; j < state[i].size(); ++j) {
            if (state[i][j].size() == 1) {
                stream << utils::getSingleCellValue(state[i][j]);
            } else if (useSimpleFormat) {
                stream << '.';
            } else {
                stream << '@';
                auto& possible_values = state[i][j];
                for (auto it = possible_values.begin(); it != possible_values.end(); ++it) {
                    stream << *it;
                    if (std::next(it) != possible_values.end()) {
                        stream << ',';
                    }
                }
                stream << '@';
            }

            ++numProcessedElements;

            if (numProcessedElements != constants::boxSize) {
                stream << ' ';
            }

            if (numProcessedElements == constants::boxSize) {
                auto box_line = stream.str();
                if (box_line.length() > boxStrings.longest) {
                    boxStrings.longest = box_line.length();
                }
                boxStrings.lines.emplace_back(box_line);
                stream.str("");
                stream.clear();
                numProcessedElements = 0;
            }
        }
    }
    return boxStrings;
}

void printState(const state_t& state, bool useSimpleFormat)
{
    const auto boxStrings = generateBoxStrings(state, useSimpleFormat);
    const auto frame_width = boxStrings.longest;

    printHeader(frame_width);

    size_t num_processed_lines = 0;
    size_t row_index = 0;
    for (const auto& line: boxStrings.lines) {
        if (num_processed_lines == 0) {
            std::cout << "║";
        }

        std::cout << std::left << std::setw(frame_width) << line;
        ++num_processed_lines;

        if (num_processed_lines == constants::boxSize) {
            std::cout << "║\n";
            num_processed_lines = 0;
            ++row_index;
            if (((row_index % constants::boxSize) == 0) && (row_index != constants::numRows)) {
                printMiddleHorizontalLine(frame_width);
            }
        } else {
            std::cout << "│";
        }
    }

    printFooter(frame_width);
}

}  // namespace display

}  // namespace sudoku
