#pragma once

#include "Board.h"

// Zestaw bitboardów dla całej pozycji.
//
// WAŻNE: struktura NIE generuje niczego samodzielnie w nowy sposób -
// jest wypełniana WYŁĄCZNIE z danych już wygenerowanych przez silnik:
//   - bitboardy figur i occupancy: Board
//   - tablice ataków:              AttackTables
//   - szachy / przypięcia:         MoveValidator::computeCheckInfo
struct Bitboards
{
    // Figury per kolor (0 = White, 1 = Black)
    Bitboard pawns[2];
    Bitboard knights[2];
    Bitboard bishops[2];
    Bitboard rooks[2];
    Bitboard queens[2];
    Bitboard kings[2];

    // Unie ataków wszystkich figur danego koloru
    Bitboard pawnAttacks[2];
    Bitboard knightAttacks[2];
    Bitboard bishopAttacks[2];
    Bitboard rookAttacks[2];
    Bitboard queenAttacks[2];

    // Figury szachujące króla koloru / figury przypięte króla koloru
    Bitboard checkers[2];
    Bitboard pinned[2];

    // Occupancy per kolor i cała plansza
    Bitboard occupied[2];
    Bitboard allOccupied;

    static int indexOf(ChessColor color)
    {
        return color == ChessColor::White ? 0 : 1;
    }

    // Wypełnia strukturę na podstawie już wygenerowanych danych planszy.
    // `withCheckInfo=false` pomija kosztowne liczenie szachów/przypięć
    // (checkers/pinned pozostają wyzerowane) - używane w gorącej ścieżce
    // generatora ruchów.
    static Bitboards compute(const Board &board, bool withCheckInfo = true);
};
