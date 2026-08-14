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