#include "Rate/Knight.h"
#include "Rate/Evaluation.h"

#include <cstdlib>
#include <algorithm>

#include "Move/AttackTables.h"
#include "Foundation/Bitboard.h"
#include "Foundation/Board.h"

namespace
{
// Outpost: skoczek na polu wspieranym przez własnego piona,
// którego nie mogą zaatakować pionki przeciwnika
bool isKnightOutpost(const Board& /*board*/, const Bitboards& bitboards, ChessColor color, int file, int rank)
{
    const int idx = Bitboards::indexOf(color);
    const int ownPawnIdx = idx;
    const int enemyPawnIdx = 1 - idx;

    const int supportingRank = rank + ((color == ChessColor::White) ? -1 : 1);

    if (supportingRank >= 0 && supportingRank < 8)
    {
        bool supported = false;
        for (int f = file - 1; f <= file + 1; f += 2)
        {
            if (f >= 0 && f < 8)
            {
                if (getBit(bitboards.pawns[ownPawnIdx], static_cast<Square>(supportingRank * 8 + f)))
                {
                    supported = true;
                    break;
                }
            }
        }
        if (!supported) return false;
    }
    else
    {
        return false;
    }

    const int pawnAttackRank1 = rank + ((color == ChessColor::White) ? 1 : -1);
    const int pawnAttackRank2 = rank + ((color == ChessColor::White) ? 2 : -2);

    for (int f = file - 1; f <= file + 1; f += 2)
    {
        if (f >= 0 && f < 8 && pawnAttackRank1 >= 0 && pawnAttackRank1 < 8)
        {
            if (getBit(bitboards.pawns[enemyPawnIdx], static_cast<Square>(pawnAttackRank1 * 8 + f)))
                return false;
        }
        if (f >= 0 && f < 8 && pawnAttackRank2 >= 0 && pawnAttackRank2 < 8)
        {
            if (getBit(bitboards.pawns[enemyPawnIdx], static_cast<Square>(pawnAttackRank2 * 8 + f)))
                return false;
        }
    }

    return true;
}

int countKnightMobility(const Bitboards& bitboards, ChessColor color, Square from)
{
    Bitboard attacks = AttackTables::knightAttacks(from);
    attacks &= ~bitboards.occupied[Bitboards::indexOf(color)];
    return countBits(attacks);
}

bool isKnightOnEdge(int file, int rank)
{
    return (file <= 1 || file >= 6 || rank <= 1 || rank >= 6);
}
}

int KnightEvaluation::evaluate(const Board& board, const Bitboards& bitboards, ChessColor color)
{
    constexpr int Material = 3200;
    constexpr int CenterBonusMax = 180;
    constexpr int EdgePenalty = 120;
    constexpr int OutpostBonusMG = 250;
    constexpr int OutpostBonusEG = 350;
    constexpr int MobBonusPerSquareMG = 40;
    constexpr int MobBonusPerSquareEG = 28;
    constexpr int MobMax = 400;
    constexpr int ConnectedKnightsBonus = 80;

    const int idx = Bitboards::indexOf(color);
    const int phase = Evaluation::gamePhase(bitboards);
    int score = 0;
    int knightCount = 0;

    Bitboard knights = bitboards.knights[idx];
    while (knights)
    {
        const Square square = popLeastSignificantBit(knights);

        ++knightCount;
        const int index = static_cast<int>(square);
        const int file = index % 8;
        const int rank = index / 8;

        score += Material;

        const double centerDist = std::abs(file - 3.5) + std::abs(rank - 3.5);
        score += CenterBonusMax - static_cast<int>(centerDist * 35);

        if (isKnightOnEdge(file, rank))
        {
            score -= EdgePenalty;
        }

        if (isKnightOutpost(board, bitboards, color, file, rank))
        {
            score += (OutpostBonusMG * phase + OutpostBonusEG * (24 - phase)) / 24;
        }

        const int mobility = countKnightMobility(bitboards, color, square);
        const int mobBonus = (MobBonusPerSquareMG * phase + MobBonusPerSquareEG * (24 - phase)) / 24;
        score += std::max(0, std::min(mobility * mobBonus, MobMax) - 50);
    }

    if (knightCount >= 2)
    {
        score += ConnectedKnightsBonus;
    }

    return score;
}
