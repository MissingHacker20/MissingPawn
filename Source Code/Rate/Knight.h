#pragma once

#include "Foundation/Board.h"

class KnightEvaluation
{
public:
    static int evaluate(const Board& board, ChessColor color);
};
