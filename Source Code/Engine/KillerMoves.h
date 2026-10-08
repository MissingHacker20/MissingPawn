/*
 * MissingPawn
 * Copyright (C) 2026 Kacper Wieczorek
 *
 * This file is part of MissingPawn.
 *
 * Licensed under the GNU General Public License v3 or later.
 * See LICENSE for details.
 */

#pragma once

#include "Foundation/Move.h"

class KillerMoves
{
public:

static constexpr int MaxDepth = 128;
    static constexpr int KillerCount = 3;

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