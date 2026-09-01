#pragma once

#include "Foundation/Bitboards.h"

class RookEvaluation
{
public:
    static int evaluate(const Board& board, const Bitboards& bitboards, ChessColor color);
};
