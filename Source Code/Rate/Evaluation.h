#pragma once

#include "Foundation/Board.h"

class Evaluation
{
public:
    // Dodatni wynik oznacza przewagę białych, ujemny — czarnych.
    static int evaluate(const Board& board);
};
