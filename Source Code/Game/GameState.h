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

class Board;

enum class GameResult
{
    Playing,
    WhiteWin,
    BlackWin,
    Stalemate,
    FiftyMoveRule,
    ThreefoldRepetition
};

class GameState
{
public:
    static GameResult getResult(const Board& board);
};
