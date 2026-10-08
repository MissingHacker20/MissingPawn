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

#include <string>

class Board;

class Fen
{
public:
    static bool load(Board& board, const std::string& fen);
};
