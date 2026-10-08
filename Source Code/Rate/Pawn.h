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

class PawnEvaluation
{
public:
    static int evaluate(const Board &board, const Bitboards &bitboards, ChessColor color);
};
