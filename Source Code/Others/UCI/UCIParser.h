#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct GoParameters
{
    int depth;

    int moveTime = 0;

    int whiteTime = 0;
    int blackTime = 0;

    int whiteIncrement = 0;
    int blackIncrement = 0;

    int movesToGo = 30;

    uint64_t nodeLimit = 0;

    int mateDepth = 0;

    bool infinite = false;

    bool ponder = false;

    std::vector<std::string> searchMoves;
};

class UCIParser
{
public:

    static GoParameters parseGo(
        const std::vector<std::string>& tokens);
};