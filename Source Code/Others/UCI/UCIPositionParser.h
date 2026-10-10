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

#include <vector>
#include <string>

class Board;

class UCIPositionParser
{
public:

    static bool parse(
        Board& board,
        const std::vector<std::string>& tokens,
        std::string& errorMessage);
};