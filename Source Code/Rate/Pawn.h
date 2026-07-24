#pragma once

#include "Foundation/Board.h"

class PawnEvaluation
{
public:
    static int evaluate(const Board& board, ChessColor color);
};
