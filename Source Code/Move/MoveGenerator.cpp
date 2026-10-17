/*
  This file contains code derived from Stockfish.

  Copyright (C) 2004-2026 The Stockfish developers
  Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published
  by the Free Software Foundation, either version 3 of the License,
  or (at your option) any later version.

  This file has been modified and adapted for the MissingPawn chess
  engine.

  Modifications:
  Copyright (C) 2026 Kacper Wieczorek
*/

#include "MoveGenerator.h"

#include "AttackTables.h"
#include "Foundation/Bitboards.h"
#include "MoveValidator.h"

namespace
{
    constexpr int colorIndex(ChessColor color)
    {
        return color == ChessColor::White ? 0 : 1;
    }

    constexpr Bitboard squareBit(Square square)
    {
        return Bitboard{1} << static_cast<int>(square);
    }

    void addPromotions(MoveList &moves, Square from, Square to, Piece pawn, Piece captured, bool capturesOnly)
    {
        const bool capture = captured != Piece::None;
        if (!capturesOnly || capture)
            moves.add(Move(from, to, pawn, capture ? MoveFlag::PromotionCaptureQueen : MoveFlag::PromotionQueen, captured));
        if (capture)
        {
            moves.add(Move(from, to, pawn, MoveFlag::PromotionCaptureRook, captured));
            moves.add(Move(from, to, pawn, MoveFlag::PromotionCaptureBishop, captured));
            moves.add(Move(from, to, pawn, MoveFlag::PromotionCaptureKnight, captured));
        }
        else if (!capturesOnly)
        {
            moves.add(Move(from, to, pawn, MoveFlag::PromotionRook));
            moves.add(Move(from, to, pawn, MoveFlag::PromotionBishop));
            moves.add(Move(from, to, pawn, MoveFlag::PromotionKnight));
        }
    }

    void addPieceMoves(const Board &board, const Bitboards &bitboards, MoveList &moves,
                       Piece piece, Bitboard sources, Bitboard target)
    {
        const Bitboard occupancy = bitboards.allOccupied;
        while (sources)
        {
            const Square from = popLeastSignificantBit(sources);
            Bitboard destinations = AttackTables::pieceAttacks(piece, from, occupancy) & target;
            while (destinations)
            {
                const Square to = popLeastSignificantBit(destinations);
                const Piece victim = board.pieceAt(to);
                moves.add(Move(from, to, piece, victim == Piece::None ? MoveFlag::Quiet : MoveFlag::Capture, victim));
            }
        }
    }

    void addPawnMoves(const Board &board, const Bitboards &bitboards, MoveList &moves,
                      ChessColor side, bool capturesOnly, bool includeQuietQueenPromotions,
                      Bitboard captureTarget)
    {
        const bool white = side == ChessColor::White;
        const Piece pawn = white ? Piece::WhitePawn : Piece::BlackPawn;
        const Bitboard enemy = bitboards.occupied[colorIndex(MoveValidator::oppositeColor(side))];
        const int step = white ? 8 : -8;
        const int startRank = white ? 1 : 6;
        const int promotionRank = white ? 7 : 0;
        Bitboard pawns = bitboards.pawns[colorIndex(side)];

        while (pawns)
        {
            const Square from = popLeastSignificantBit(pawns);
            const int fromIndex = static_cast<int>(from);
            const int file = fromIndex % 8;
            const int rank = fromIndex / 8;

             const int oneIndex = fromIndex + step;
             if (oneIndex >= 0 && oneIndex < 64 &&
                 !(bitboards.allOccupied & (Bitboard{1} << oneIndex))) {
                 const Square one = static_cast<Square>(oneIndex);
                 if (oneIndex / 8 == promotionRank) {
                     if (includeQuietQueenPromotions) {
                         moves.add(Move(from, one, pawn, MoveFlag::PromotionQueen));
                     } else {
                         addPromotions(moves, from, one, pawn, Piece::None, false);
                     }
                 } else {
                     if (!capturesOnly) {
                         moves.add(Move(from, one, pawn, MoveFlag::Quiet));
                         const int twoIndex = fromIndex + 2 * step;
                         if (rank == startRank && !(bitboards.allOccupied & (Bitboard{1} << twoIndex))) {
                             moves.add(Move(from, static_cast<Square>(twoIndex), pawn, MoveFlag::DoublePawnPush));
                         }
                     }
                 }
             }

            for (const int fileDelta : {-1, 1})
            {
                const int targetFile = file + fileDelta;
                const int targetIndex = fromIndex + step + fileDelta;
                if (targetFile < 0 || targetFile >= 8 || targetIndex < 0 || targetIndex >= 64)
                    continue;

                const Square to = static_cast<Square>(targetIndex);
                const Piece victim = board.pieceAt(to);
                if (victim != Piece::None && (enemy & squareBit(to)) &&
                    (!capturesOnly || (captureTarget & squareBit(to))))
                {
                    if (targetIndex / 8 == promotionRank)
                        addPromotions(moves, from, to, pawn, victim, false);
                    else
                        moves.add(Move(from, to, pawn, MoveFlag::Capture, victim));
                }
            }

            const Square ep = board.getEnPassantSquare();
            if (ep != Square::None)
            {
                const int epIndex = static_cast<int>(ep);
                const int epFile = epIndex % 8;
                const int epRank = epIndex / 8;
                const int capturedIndex = epIndex + (white ? -8 : 8);
                const Piece capturedPawn = white ? Piece::BlackPawn : Piece::WhitePawn;
                if (std::abs(epFile - file) == 1 && epRank == rank + (white ? 1 : -1) &&
                    board.pieceAt(ep) == Piece::None && capturedIndex >= 0 && capturedIndex < 64 &&
                    board.pieceAt(static_cast<Square>(capturedIndex)) == capturedPawn &&
                    (!capturesOnly || (captureTarget & squareBit(ep))))
                    moves.add(Move(from, ep, pawn, MoveFlag::EnPassant, capturedPawn));
            }
        }
    }

    void addCastlingMoves(const Board &board, MoveList &moves)
    {
        const ChessColor side = board.getSideToMove();
        const bool white = side == ChessColor::White;
        const Piece king = white ? Piece::WhiteKing : Piece::BlackKing;
        const Piece rook = white ? Piece::WhiteRook : Piece::BlackRook;
        const Square home = white ? Square::E1 : Square::E8;
        const Square rookKingSide = white ? Square::H1 : Square::H8;
        const Square rookQueenSide = white ? Square::A1 : Square::A8;
        const Square f = white ? Square::F1 : Square::F8;
        const Square g = white ? Square::G1 : Square::G8;
        const Square b = white ? Square::B1 : Square::B8;
        const Square c = white ? Square::C1 : Square::C8;
        const Square d = white ? Square::D1 : Square::D8;
        const uint8_t kingSideRight = white ? 0b0001 : 0b0100;
        const uint8_t queenSideRight = white ? 0b0010 : 0b1000;

        if (board.pieceAt(home) != king)
            return;
        if ((board.getCastlingRights() & kingSideRight) && board.pieceAt(rookKingSide) == rook &&
            board.pieceAt(f) == Piece::None && board.pieceAt(g) == Piece::None)
            moves.add(Move(home, g, king, MoveFlag::KingCastle));
        if ((board.getCastlingRights() & queenSideRight) && board.pieceAt(rookQueenSide) == rook &&
            board.pieceAt(b) == Piece::None && board.pieceAt(c) == Piece::None && board.pieceAt(d) == Piece::None)
            moves.add(Move(home, c, king, MoveFlag::QueenCastle));
    }

    void filterLegalMoves(const Board &board, MoveList &moves)
    {
        const int generatedCount = moves.size();
        int legalCount = 0;
        for (int read = 0; read < generatedCount; ++read)
            if (MoveValidator::isMoveLegalSlow(board, moves[read]))
                moves[legalCount++] = moves[read];
        moves.setSize(legalCount);
    }
}

void MoveGenerator::generateMoves(const Board &board, MoveList &moves,
                                  const MoveValidator::CheckInfo &)
{
    const Bitboards bitboards = Bitboards::computeLight(board);
    generateMoves(board, bitboards, moves, MoveValidator::CheckInfo{});
}

void MoveGenerator::generateMoves(const Board &board, const Bitboards &bitboards, MoveList &moves,
                                  const MoveValidator::CheckInfo &)
{
    moves.clear();
    const ChessColor side = board.getSideToMove();
    const int index = colorIndex(side);
    const Bitboard target = ~bitboards.occupied[index];

    addPawnMoves(board, bitboards, moves, side, false, false, target);
    addPieceMoves(board, bitboards, moves, side == ChessColor::White ? Piece::WhiteKnight : Piece::BlackKnight,
                  bitboards.knights[index], target);
    addPieceMoves(board, bitboards, moves, side == ChessColor::White ? Piece::WhiteBishop : Piece::BlackBishop,
                  bitboards.bishops[index], target);
    addPieceMoves(board, bitboards, moves, side == ChessColor::White ? Piece::WhiteRook : Piece::BlackRook,
                  bitboards.rooks[index], target);
    addPieceMoves(board, bitboards, moves, side == ChessColor::White ? Piece::WhiteQueen : Piece::BlackQueen,
                  bitboards.queens[index], target);

    // King destinations are candidate squares; exact safety is checked below.
    const Square kingSquare = board.getKingSquare(side);
    if (kingSquare != Square::None)
        addPieceMoves(board, bitboards, moves, side == ChessColor::White ? Piece::WhiteKing : Piece::BlackKing,
                      bitboards.kings[index], AttackTables::kingAttacks(kingSquare) & ~bitboards.occupied[index]);
    addCastlingMoves(board, moves);
    filterLegalMoves(board, moves);
}

void MoveGenerator::generateCaptures(const Board &board, MoveList &moves,
                                     const MoveValidator::CheckInfo &)
{
    const Bitboards bitboards = Bitboards::computeLight(board);
    generateCaptures(board, bitboards, moves, MoveValidator::CheckInfo{});
}

void MoveGenerator::generateLegalCaptures(const Board &board, MoveList &moves,
                                          const MoveValidator::CheckInfo &checkInfo)
{
    generateCaptures(board, moves, checkInfo);
}

void MoveGenerator::generateCaptures(const Board &board, const Bitboards &bitboards, MoveList &moves,
                                     const MoveValidator::CheckInfo &)
{
    moves.clear();
    const ChessColor side = board.getSideToMove();
    const int index = colorIndex(side);
    const int enemyIndex = colorIndex(MoveValidator::oppositeColor(side));
    const Bitboard enemy = bitboards.occupied[enemyIndex] &
                           ~(side == ChessColor::White ? bitboards.kings[1] : bitboards.kings[0]);

    // Captures include all captures and queen promotions, matching Stockfish's capture stage.
    addPawnMoves(board, bitboards, moves, side, true, true, enemy);
    addPieceMoves(board, bitboards, moves, side == ChessColor::White ? Piece::WhiteKnight : Piece::BlackKnight,
                  bitboards.knights[index], enemy);
    addPieceMoves(board, bitboards, moves, side == ChessColor::White ? Piece::WhiteBishop : Piece::BlackBishop,
                  bitboards.bishops[index], enemy);
    addPieceMoves(board, bitboards, moves, side == ChessColor::White ? Piece::WhiteRook : Piece::BlackRook,
                  bitboards.rooks[index], enemy);
    addPieceMoves(board, bitboards, moves, side == ChessColor::White ? Piece::WhiteQueen : Piece::BlackQueen,
                  bitboards.queens[index], enemy);
    const Square kingSquare = board.getKingSquare(side);
    if (kingSquare != Square::None)
        addPieceMoves(board, bitboards, moves, side == ChessColor::White ? Piece::WhiteKing : Piece::BlackKing,
                      bitboards.kings[index], AttackTables::kingAttacks(kingSquare) & enemy);

    // A quiet queen promotion is part of Stockfish's CAPTURES stage, so retain it here.
    // en passant was generated by addPawnMoves even though its destination is empty.
    filterLegalMoves(board, moves);
}
