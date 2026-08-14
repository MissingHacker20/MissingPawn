#pragma once

#include <cstdint>
#include <bit>

#include "Square.h"

using Bitboard = uint64_t;

// Ustawia bit na wybranym polu
inline void setBit(Bitboard& board, Square square)
{
    board |= (1ULL << static_cast<int>(square));
}

// Czyści bit na wybranym polu
inline void clearBit(Bitboard& board, Square square)
{
    board &= ~(1ULL << static_cast<int>(square));
}

// Sprawdza, czy bit jest ustawiony
inline bool getBit(Bitboard board, Square square)
{
    return board & (1ULL << static_cast<int>(square));
}

inline int countBits(Bitboard board)
{
    return std::popcount(board);
}

inline Square popLeastSignificantBit(Bitboard& board)
{
    Square square = static_cast<Square>(std::countr_zero(board));

    board &= board - 1;

    return square;
}

