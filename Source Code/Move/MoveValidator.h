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

#include "Board.h"
#include "MoveList.h"
#include "Foundation/Bitboard.h"

class MoveValidator
{
public:

    struct CheckInfo
    {
        Bitboard checkers = 0;           // Wszystkie figury dające szach
        Bitboard pinned = 0;             // Nasze figury przybite do króla
        Bitboard evasionMask = ~Bitboard(0); // Pola dostępne przy odpowiedzi na szacha
        Bitboard pinRays[64]{};           // Dla każdej przybitej figury: pola na których może się ruszyć
        Square kingSquare = Square::None;
        bool inCheck = false;
        bool doubleCheck = false;
        Bitboard enemyAttacks = 0;       // Wszystkie pola atakowane przez przeciwnika
    };

    // Oblicza informacje o szachach i przyspieszaniach dla danej strony.
    // Zwraca strukturę CheckInfo z gotowymi maskami.
    static CheckInfo computeCheckInfo(const Board& board, ChessColor side);

    // Ocena taktyczna (SEE / wiszące figury) dla całej planszy.
    // Zwraca wynik netto (biały - czarny): kara za "wiszące" figury
    // strony, której figura należy.
    static int evaluateTactics(
        const Board& board
    );

    // Static Exchange Evaluation (SEE): zwraca bilans wymiany na danym polu
    // (np. -2200 gdy skoczek 3200 jest bity pionkiem, a my odbijamy pionka 1000).
    // Dodatni = wymiana korzystna dla strony do ruchu; ujemna = stratna;
    // 0 = wyrównana.
    static int see(
        const Board& board,
        Square square);

    // Static Exchange Evaluation for a specific legal capture move.
    static int see(
        const Board& board,
        const Move& move);

    static void updatePieceBitboards(
        Board& board,
        const MoveList& legalMoves);

    // Legacy - kept for debug/validation only
    static bool isKingInCheck(
        const Board& board,
        ChessColor side
    );

    static bool isSquareAttacked(
        const Board& board,
        Square square,
        ChessColor attacker
    );

static ChessColor oppositeColor(ChessColor color);

// Legacy functions - kept for compatibility but should not be used in new code
    static void filterLegalMoves(const Board& board, MoveList& moveList);
    static bool isMoveLegal(const Board& board, const Move& move);
    static bool isMoveLegal(const Board& board, const Move& move, const CheckInfo& checkInfo);
    static bool isMoveLegalSlow(const Board& board, const Move& move);

private:

    static Square findKing(
        const Board& board,
        ChessColor side
    );

    static int pieceValue(Piece piece);

    static Piece leastValuableAttacker(
        const Board& board,
        Square square,
        ChessColor attacker,
        Square& fromSquare);

    static int seeRecursive(
        Board& board,
        Square square,
        ChessColor sideToMove,
        int value);

    static int seeCaptureSequence(
        Board& board,
        Square square,
        int depth);

    static bool leastLegalAttacker(
        const Board& board,
        Square square,
        Move& move);

    static bool isStraightAttacker(
        const Board& board,
        Square from,
        int targetFile,
        int targetRank);

    static bool isDiagonalAttacker(
        const Board& board,
        Square from,
        int targetFile,
        int targetRank);
};
