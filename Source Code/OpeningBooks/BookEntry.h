#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Foundation/Board.h"
#include "Foundation/Move.h"
#include "Foundation/Square.h"
#include "Foundation/Piece.h"

// Struktura przechowująca pojedynczy wpis w książce otwarć
struct BookEntry
{
    uint64_t key;          // Zobrist hash pozycji
    uint16_t encodedMove;  // Zakodowany ruch (from:6 + to:6 + promocja:4)
    uint16_t weight;       // Waga (popularność) - im wyższa, tym częściej wybierany
};

// Konwersja Move na 16-bitowy kod
inline uint16_t moveToCode(const Move& move)
{
    uint16_t code = 0;
    code |= static_cast<uint16_t>(move.from) & 0x3F;       // bits 0-5: from
    code |= (static_cast<uint16_t>(move.to) & 0x3F) << 6;   // bits 6-11: to

    uint16_t promo = 0;
    switch (move.flag)
    {
    case MoveFlag::PromotionKnight:
    case MoveFlag::PromotionCaptureKnight:
        promo = 1; break;
    case MoveFlag::PromotionBishop:
    case MoveFlag::PromotionCaptureBishop:
        promo = 2; break;
    case MoveFlag::PromotionRook:
    case MoveFlag::PromotionCaptureRook:
        promo = 3; break;
    case MoveFlag::PromotionQueen:
    case MoveFlag::PromotionCaptureQueen:
        promo = 4; break;
    default:
        promo = 0; break;
    }
    code |= promo << 12;

    return code;
}

// Helper: dodaje pojedynczy wpis (hash pozycji -> ruch)
inline void addBookEntry(
    std::vector<BookEntry>& book,
    const Board& board,
    const Move& move,
    uint16_t weight)
{
    BookEntry entry;
    entry.key = board.getZobristKey();
    entry.encodedMove = moveToCode(move);
    entry.weight = weight;
    book.push_back(entry);
}

// Konwersja 16-bitowego kodu na Move
inline Move codeToMove(const Board& board, uint16_t code)
{
    Square from = static_cast<Square>(code & 0x3F);
    Square to = static_cast<Square>((code >> 6) & 0x3F);
    uint16_t promo = (code >> 12) & 0x0F;

    Piece piece = board.pieceAt(from);

    MoveFlag flag = MoveFlag::Quiet;
    Piece captured = board.pieceAt(to);

    if (captured != Piece::None)
    {
        flag = MoveFlag::Capture;
    }

    // Sprawdź czy to en passant
    if (piece == Piece::WhitePawn || piece == Piece::BlackPawn)
    {
        if (board.getEnPassantSquare() == to)
        {
            flag = MoveFlag::EnPassant;
            captured = (piece == Piece::WhitePawn) ? Piece::BlackPawn : Piece::WhitePawn;
        }
    }

    // Sprawdź promocję
    switch (promo)
    {
    case 1:
        flag = (flag == MoveFlag::Capture) ? MoveFlag::PromotionCaptureKnight : MoveFlag::PromotionKnight;
        break;
    case 2:
        flag = (flag == MoveFlag::Capture) ? MoveFlag::PromotionCaptureBishop : MoveFlag::PromotionBishop;
        break;
    case 3:
        flag = (flag == MoveFlag::Capture) ? MoveFlag::PromotionCaptureRook : MoveFlag::PromotionRook;
        break;
    case 4:
        flag = (flag == MoveFlag::Capture) ? MoveFlag::PromotionCaptureQueen : MoveFlag::PromotionQueen;
        break;
    default:
        break;
    }

    // Sprawdź roszadę
    if (piece == Piece::WhiteKing && from == Square::E1)
    {
        if (to == Square::G1) flag = MoveFlag::KingCastle;
        else if (to == Square::C1) flag = MoveFlag::QueenCastle;
    }
    if (piece == Piece::BlackKing && from == Square::E8)
    {
        if (to == Square::G8) flag = MoveFlag::KingCastle;
        else if (to == Square::C8) flag = MoveFlag::QueenCastle;
    }

    // Sprawdź double pawn push (porównaj indeksy pól)
    int fromIdx = static_cast<int>(from);
    int toIdx = static_cast<int>(to);
    if (piece == Piece::WhitePawn && toIdx == fromIdx + 16)
        flag = MoveFlag::DoublePawnPush;
    if (piece == Piece::BlackPawn && toIdx == fromIdx - 16)
        flag = MoveFlag::DoublePawnPush;

    return Move(from, to, piece, flag, captured);
}
