#pragma once

#include "Foundation/Board.h"

class RookEvaluation
{
public:
    static int evaluate(const Board& board, ChessColor color);
};
