#pragma once

#include "Foundation/Board.h"

class KingEvaluation
{
public:
    static int evaluate(const Board& board, ChessColor color);
};
