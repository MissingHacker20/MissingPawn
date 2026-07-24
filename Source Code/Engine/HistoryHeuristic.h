#pragma once

#include "Foundation/Color.h"
#include "Foundation/Move.h"

class HistoryHeuristic
{
public:

    static void add(
        ChessColor side,
        const Move& move,
        int depth);

    static int get(
        ChessColor side,
        const Move& move);

    static void clear();


private:

    static int history[2][64][64];
};