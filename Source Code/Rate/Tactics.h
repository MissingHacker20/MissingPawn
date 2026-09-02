#pragma once
#include "Foundation/Board.h"

class TacticsEvaluation
{
public:
    // Wynik netto w milipionach (biały - czarny).
    static int evaluate(const Board &board);
};
