#include "Rate/Bishop.h"

#include <cstdlib>
#include <algorithm>

#include "Move/AttackTables.h"
#include "Foundation/Bitboard.h"
#include "Foundation/Board.h"

namespace
{
int countBishopMobility(const Bitboards& bitboards, ChessColor color, Square from)
{
    Bitboard attacks = AttackTables::bishopAttacks(from, bitboards.allOccupied);
    attacks &= ~bitboards.occupied[Bitboards::indexOf(color)];
    return countBits(attacks);
}

// Sprawdza czy goniec jest "fianchetto" - na b7/g7/b2/g2 z własnym pionem przed
bool isFianchetto(const Bitboards& bitboards, ChessColor color, int file, int rank)
{
    if (!(file == 1 || file == 6)) return false; // b lub g

    const int expectedRank = (color == ChessColor::White) ? 1 : 6;
    if (rank != expectedRank) return false;

    // Sprawdź czy pion przed gońcem
    const int pawnRank = rank + ((color == ChessColor::White) ? 1 : -1);
    if (pawnRank >= 0 && pawnRank < 8)
    {
        return getBit(bitboards.pawns[Bitboards::indexOf(color)],
                      static_cast<Square>(pawnRank * 8 + file));
    }
    return false;
}

// Sprawdza czy goniec jest "wielkim" (długi diagonale)
bool isLongDiagonalBishop(int file, int rank)
{
    return (file == rank) || (file + rank == 7);
}
}

int BishopEvaluation::evaluate(const Board& /*board*/, const Bitboards& bitboards, ChessColor color)
{
    constexpr int Material = 330;
    constexpr int BishopPairBonus = 40;
    constexpr int FianchettoBonus = 20;
    constexpr int LongDiagBonus = 10;
    constexpr int MobBonusPerSquare = 3;
    constexpr int MobMax = 50;

    const int idx = Bitboards::indexOf(color);
    int score = 0;
    int count = 0;

    Bitboard bishops = bitboards.bishops[idx];
    while (bishops)
    {
        const Square square = popLeastSignificantBit(bishops);

        ++count;
        const int index = static_cast<int>(square);
        const int file = index % 8;
        const int rank = index / 8;

        score += Material;

        // Centrum
        const double centerDist = std::abs(file - 3.5) + std::abs(rank - 3.5);
        score += 20 - static_cast<int>(centerDist * 3);

        // Fianchetto
        if (isFianchetto(bitboards, color, file, rank))
        {
            score += FianchettoBonus;
        }

        // Długa diagonala
        if (isLongDiagonalBishop(file, rank))
        {
            score += LongDiagBonus;
        }

        // Mobilność
        const int mobility = countBishopMobility(bitboards, color, square);
        score += std::min(mobility * MobBonusPerSquare, MobMax);
    }

    // Para gońców
    if (count >= 2)
    {
        score += BishopPairBonus;
    }

    return score;
}
