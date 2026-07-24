#pragma once

#include "Foundation/Bitboard.h"

namespace AttackTables
{
inline Bitboard whitePawnAttacks(Square square)
{
    const int index = static_cast<int>(square);
    const int file = index % 8;
    const int rank = index / 8;
    Bitboard attacks = 0;

    if (rank < 7)
    {
        if (file > 0)
            setBit(attacks, static_cast<Square>(index + 7));
        if (file < 7)
            setBit(attacks, static_cast<Square>(index + 9));
    }

    return attacks;
}

inline Bitboard blackPawnAttacks(Square square)
{
    const int index = static_cast<int>(square);
    const int file = index % 8;
    const int rank = index / 8;
    Bitboard attacks = 0;

    if (rank > 0)
    {
        if (file > 0)
            setBit(attacks, static_cast<Square>(index - 9));
        if (file < 7)
            setBit(attacks, static_cast<Square>(index - 7));
    }

    return attacks;
}

inline Bitboard knightAttacks(Square square)
{
    constexpr int offsets[8][2] =
    {
        { 1, 2}, { 2, 1}, { 2, -1}, { 1, -2},
        {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2}
    };

    const int index = static_cast<int>(square);
    const int file = index % 8;
    const int rank = index / 8;
    Bitboard attacks = 0;

    for (const auto& offset : offsets)
    {
        const int targetFile = file + offset[0];
        const int targetRank = rank + offset[1];
        if (targetFile >= 0 && targetFile < 8 &&
            targetRank >= 0 && targetRank < 8)
        {
            setBit(attacks, static_cast<Square>(targetRank * 8 + targetFile));
        }
    }

    return attacks;
}

inline Bitboard kingAttacks(Square square)
{
    const int index = static_cast<int>(square);
    const int file = index % 8;
    const int rank = index / 8;
    Bitboard attacks = 0;

    for (int fileDelta = -1; fileDelta <= 1; ++fileDelta)
    {
        for (int rankDelta = -1; rankDelta <= 1; ++rankDelta)
        {
            if (fileDelta == 0 && rankDelta == 0)
                continue;

            const int targetFile = file + fileDelta;
            const int targetRank = rank + rankDelta;
            if (targetFile >= 0 && targetFile < 8 &&
                targetRank >= 0 && targetRank < 8)
            {
                setBit(attacks, static_cast<Square>(targetRank * 8 + targetFile));
            }
        }
    }

    return attacks;
}
}
