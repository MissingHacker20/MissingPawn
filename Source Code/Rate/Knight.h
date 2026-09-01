#pragma once

#include "Foundation/Bitboards.h"

class KnightEvaluation
{
public:
    static int evaluate(const Board& board, const Bitboards& bitboards, ChessColor color);
};
