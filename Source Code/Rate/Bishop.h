#pragma once

#include "Foundation/Bitboards.h"

class BishopEvaluation
{
public:
    static int evaluate(const Board& board, const Bitboards& bitboards, ChessColor color);
};
