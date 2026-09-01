#pragma once

#include "Foundation/Bitboards.h"

class PawnEvaluation
{
public:
    static int evaluate(const Board &board, const Bitboards &bitboards, ChessColor color);
};
