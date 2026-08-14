#pragma once

#include <string>
#include <vector>

#include "Foundation/Move.h"
#include "Foundation/Board.h"

class MoveParser
{
public:
    // Parsuje notację UCI (np. "e2e4", "g1f3", "e7e8q") na obiekt Move
    static bool parseMove(
        const Board& board,
        const std::string& uciString,
        Move& outMove);

    // Parsuje listę ruchów UCI
    static std::vector<Move> parseMoveList(
        const Board& board,
        const std::vector<std::string>& uciMoves);
};
