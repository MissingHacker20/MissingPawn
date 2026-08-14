#pragma once

#include "Foundation/Board.h"

class BishopEvaluation
{
public:
    static int evaluate(const Board& board, ChessColor color);
};
