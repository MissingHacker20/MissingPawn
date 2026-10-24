/*
 * MissingPawn
 * Copyright (C) 2026 Kacper Wieczorek
 *
 * This file is part of MissingPawn.
 *
 * Licensed under the GNU General Public License v3 or later.
 * See LICENSE for details.
 */

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

std::pair<int, int> KnightEvaluation::evaluatePair(const Board& board, const Bitboards& bitboards, ChessColor color)
{
    constexpr int MaterialMG = 3387;
    constexpr int MaterialEG = 3272;
    constexpr int CenterBonusMaxMG = 271;
    constexpr int CenterBonusMaxEG = 277;
    constexpr int EdgePenaltyMG = 55;
    constexpr int EdgePenaltyEG = 4;
    constexpr int OutpostBonusMG = 324;
    constexpr int OutpostBonusEG = 263;
    constexpr int MobBonusPerSquareMG = 129;
    constexpr int MobBonusPerSquareEG = 43;
    constexpr int MobMaxMG = 400;
    constexpr int MobMaxEG = 400;
    constexpr int MobilityFloorMG = 50;
    constexpr int MobilityFloorEG = 50;
    constexpr int CenterDistanceWeightMG = 0;
    constexpr int CenterDistanceWeightEG = 61;
    constexpr int ConnectedKnightsBonusMG = 170;
    constexpr int ConnectedKnightsBonusEG = 138;

    const int idx = Bitboards::indexOf(color);
    int mgScore = 0;
    int egScore = 0;
    int knightCount = 0;

    Bitboard knights = bitboards.knights[idx];
    while (knights)
    {
        const Square square = popLeastSignificantBit(knights);

        ++knightCount;
        const int index = static_cast<int>(square);
        const int file = index % 8;
        const int rank = index / 8;

        mgScore += MaterialMG;
        egScore += MaterialEG;

        const double centerDist = std::abs(file - 3.5) + std::abs(rank - 3.5);
        mgScore += CenterBonusMaxMG - static_cast<int>(centerDist * CenterDistanceWeightMG);
        egScore += CenterBonusMaxEG - static_cast<int>(centerDist * CenterDistanceWeightEG);

        if (isKnightOnEdge(file, rank))
        {
            mgScore -= EdgePenaltyMG;
            egScore -= EdgePenaltyEG;
        }

        if (isKnightOutpost(board, bitboards, color, file, rank))
        {
            mgScore += OutpostBonusMG;
            egScore += OutpostBonusEG;
        }

        const int mobility = countKnightMobility(bitboards, color, square);
        mgScore += std::max(0, std::min(mobility * MobBonusPerSquareMG, MobMaxMG) - MobilityFloorMG);
        egScore += std::max(0, std::min(mobility * MobBonusPerSquareEG, MobMaxEG) - MobilityFloorEG);
    }

    if (knightCount >= 2)
    {
        mgScore += ConnectedKnightsBonusMG;
        egScore += ConnectedKnightsBonusEG;
    }

    return {mgScore, egScore};
}

int KnightEvaluation::evaluate(const Board& board, const Bitboards& bitboards, ChessColor color)
{
    const auto [mg, eg] = evaluatePair(board, bitboards, color);
    const int phase = Evaluation::gamePhase(bitboards);
    return (mg * phase + eg * (24 - phase)) / 24;
}
