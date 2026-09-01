#pragma once

#include <array>

#include "Foundation/Bitboard.h"
#include "Foundation/Piece.h"
#include "Foundation/Square.h"

namespace AttackTables
{

inline Bitboard whitePawnAttacks(Square square)
{
    extern std::array<Bitboard, 64> whitePawnAttackTable;
    return whitePawnAttackTable[static_cast<int>(square)];
}

inline Bitboard blackPawnAttacks(Square square)
{
    extern std::array<Bitboard, 64> blackPawnAttackTable;
    return blackPawnAttackTable[static_cast<int>(square)];
}

inline Bitboard knightAttacks(Square square)
{
    extern std::array<Bitboard, 64> knightAttackTable;
    return knightAttackTable[static_cast<int>(square)];
}

inline Bitboard kingAttacks(Square square)
{
    extern std::array<Bitboard, 64> kingAttackTable;
    return kingAttackTable[static_cast<int>(square)];
}

Bitboard bishopAttacks(Square square, Bitboard occupancy);

Bitboard rookAttacks(Square square, Bitboard occupancy);

Bitboard queenAttacks(Square square, Bitboard occupancy);

// Ataki figury stojącej na `square` przy zadanym occupancy.
// Dysponuje istniejącymi tablicami - niczego nie generuje od nowa.
inline Bitboard pieceAttacks(Piece piece, Square square, Bitboard occupancy)
{
    switch (piece)
    {
    case Piece::WhitePawn:   return whitePawnAttacks(square);
    case Piece::BlackPawn:   return blackPawnAttacks(square);
    case Piece::WhiteKnight:
    case Piece::BlackKnight: return knightAttacks(square);
    case Piece::WhiteKing:
    case Piece::BlackKing:   return kingAttacks(square);
    case Piece::WhiteBishop:
    case Piece::BlackBishop: return bishopAttacks(square, occupancy);
    case Piece::WhiteRook:
    case Piece::BlackRook:   return rookAttacks(square, occupancy);
    case Piece::WhiteQueen:
    case Piece::BlackQueen:  return queenAttacks(square, occupancy);
    default:                 return 0;
    }
}

void initAttackTables();

// Verifies the generated sliding-piece attack tables against the brute-force
// reference for every square and relevant occupancy. Returns true if correct.
bool verifyAttackTables();

}
