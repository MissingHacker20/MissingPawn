#pragma once

#include "Foundation/Move.h"

class KillerMoves
{
public:

static constexpr int MaxDepth = 128;
    static constexpr int KillerCount = 2;

    static void clear();

    static void add(
        int depth,
        const Move& move);

    static int score(
        int depth,
        const Move& move);

private:

    static Move killers[MaxDepth][KillerCount];
};