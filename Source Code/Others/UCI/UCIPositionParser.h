#pragma once

#include <vector>
#include <string>

class Board;

class UCIPositionParser
{
public:

    static void parse(
        Board& board,
        const std::vector<std::string>& tokens);
};