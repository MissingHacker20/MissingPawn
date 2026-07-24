#pragma once

#include <iostream>
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

inline void printBitboard(Bitboard board)
{
    std::cout << '\n';

    for (int rank = 7; rank >= 0; rank--)
    {
        std::cout << rank + 1 << "  ";

        for (int file = 0; file < 8; file++)
        {
            Square square = static_cast<Square>(rank * 8 + file);

            std::cout << (getBit(board, square) ? "X " : ". ");
        }

        std::cout << '\n';
    }

    std::cout << "\n   A B C D E F G H\n\n";
}

inline Square popLeastSignificantBit(Bitboard& board)
{
    Square square = static_cast<Square>(std::countr_zero(board));

    board &= board - 1;

    return square;
}