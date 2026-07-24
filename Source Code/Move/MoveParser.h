#pragma once

#include <string>

class Board;
class Move;

class MoveParser
{
public:

    static bool parseMove(
        const Board& board,
        const std::string& uciString,
        Move& outMove);
};

