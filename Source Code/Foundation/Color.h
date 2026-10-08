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

#include "Piece.h"

#include <cassert>

enum class ChessColor
{
    White,
    Black,

    Count
};

inline ChessColor getPieceColor(Piece piece)
{
    assert(piece != Piece::None);

    if (piece >= Piece::WhitePawn &&
        piece <= Piece::WhiteKing)
    {
        return ChessColor::White;
    }

    return ChessColor::Black;
}