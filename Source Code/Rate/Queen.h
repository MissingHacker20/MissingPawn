#pragma once

#include "Foundation/Bitboards.h"

class QueenEvaluation
{
public:
    static int evaluate(const Board& board, const Bitboards& bitboards, ChessColor color);
};
