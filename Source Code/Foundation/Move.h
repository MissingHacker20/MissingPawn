#pragma once

#include "Color.h"
#include "Square.h"
#include "Piece.h"

#include <cstdint>
#include <string>

struct UndoInfo
{
    Piece capturedPiece = Piece::None;
    uint8_t castlingRights = 0;
    Square enPassantSquare = Square::None;
    ChessColor sideToMove = ChessColor::White;
    int halfmoveClock = 0;
    int fullmoveNumber = 0;
    bool wasNullMove = false;
};

enum class MoveFlag
{
    Quiet,

    Capture,

    DoublePawnPush,

    KingCastle,
    QueenCastle,

    EnPassant,

    PromotionKnight,
    PromotionBishop,
    PromotionRook,
    PromotionQueen,

    PromotionCaptureKnight,
    PromotionCaptureBishop,
    PromotionCaptureRook,
    PromotionCaptureQueen
};

class Move
{
public:

    Square from = Square::None;
    Square to = Square::None;

    Piece piece = Piece::None;

    Piece capturedPiece = Piece::None;

    MoveFlag flag = MoveFlag::Quiet;

    Move() = default;

    Move(
    Square from,
    Square to,
    Piece piece,
    MoveFlag flag,
    Piece capturedPiece = Piece::None)
    :
    from(from),
    to(to),
    piece(piece),
    capturedPiece(capturedPiece),
    flag(flag)
    {
    }

    std::string toUCI() const
    {

        std::string text =
            squareToString(from) +
            squareToString(to);

        switch (flag)
        {
            case MoveFlag::PromotionKnight:
            case MoveFlag::PromotionCaptureKnight:
                text += 'n';
                break;

            case MoveFlag::PromotionBishop:
            case MoveFlag::PromotionCaptureBishop:
                text += 'b';
                break;

            case MoveFlag::PromotionRook:
            case MoveFlag::PromotionCaptureRook:
                text += 'r';
                break;

            case MoveFlag::PromotionQueen:
            case MoveFlag::PromotionCaptureQueen:
                text += 'q';
                break;

            default:
                break;
        }

        return text;
    }

    bool operator==(const Move& other) const
    {
        return from == other.from &&
               to == other.to &&
               piece == other.piece &&
               capturedPiece == other.capturedPiece &&
               flag == other.flag;
    }
};