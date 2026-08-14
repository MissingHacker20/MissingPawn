#pragma once

#include <array>

#include "Foundation/Bitboard.h"
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

void initAttackTables();

// Verifies the generated sliding-piece attack tables against the brute-force
// reference for every square and relevant occupancy. Returns true if correct.
bool verifyAttackTables();

}
