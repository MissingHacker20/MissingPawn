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

#include "Foundation/Bitboards.h"

class Evaluation
{
public:
    // Dodatni wynik oznacza przewagę białych, ujemny — czarnych.
    static int evaluate(const Board& board);

    // Wariant z gotowym zestawem Bitboards (bez ponownego liczenia).
    static int evaluate(const Board& board, const Bitboards& bitboards);

    // Wspólna faza taperowania: 24 = pełny middlegame, 0 = pełny endgame.
    static int gamePhase(const Bitboards& bitboards);
};
