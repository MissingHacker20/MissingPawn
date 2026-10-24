/*
 * MissingPawn
 * Copyright (C) 2026 Kacper Wieczorek
 *
 * This file is part of MissingPawn.
 *
 * Licensed under the GNU General Public License v3 or later.
 * See LICENSE for details.
 */

#include "Rate/Queen.h"
#include "Rate/Evaluation.h"

#include <algorithm>

#include "Move/AttackTables.h"
#include "Foundation/Bitboard.h"
#include "Foundation/Board.h"

namespace
{
constexpr int QueenMaterialMG = 9180;
constexpr int QueenMaterialEG = 9179;
constexpr int EarlyQueenPenaltyMG = 45;
constexpr int EarlyQueenPenaltyEG = 95;
constexpr int QueenMobilityPerSquareMG = 45;
constexpr int QueenMobilityPerSquareEG = 95;
constexpr int QueenMobilityCapMG = 400;
constexpr int QueenMobilityCapEG = 400;
constexpr int QueenMobilityOffsetMG = 50;
constexpr int QueenMobilityOffsetEG = 50;
constexpr int QueenCenterBonusMG = 158;
constexpr int QueenCenterBonusEG = 186;
int countQueenMobility(const Bitboards& bitboards, ChessColor color, Square from)
{
    Bitboard attacks = AttackTables::queenAttacks(from, bitboards.allOccupied);
    attacks &= ~bitboards.occupied[Bitboards::indexOf(color)];
    return countBits(attacks);
}

bool queenIsExposedEarly(const Bitboards& bitboards, ChessColor color, Square square)
{
    const int idx = Bitboards::indexOf(color);
    const int enemyIdx = 1 - idx;
    const int rank = static_cast<int>(square) / 8;
    const int developmentRank = color == ChessColor::White ? rank : 7 - rank;
    if (developmentRank < 2)
    {
        return false;
    }

    constexpr Bitboard WhiteMinorHomes = (1ULL << 1) | (1ULL << 6) | (1ULL << 2) | (1ULL << 5);
    constexpr Bitboard BlackMinorHomes = (1ULL << 57) | (1ULL << 62) | (1ULL << 58) | (1ULL << 61);
    const Bitboard minorHomes = color == ChessColor::White ? WhiteMinorHomes : BlackMinorHomes;
    const int undevelopedMinors = countBits((bitboards.knights[idx] | bitboards.bishops[idx]) & minorHomes);
    const Bitboard enemyAttacks = bitboards.pawnAttacks[enemyIdx]
        | bitboards.knightAttacks[enemyIdx]
        | bitboards.bishopAttacks[enemyIdx]
        | bitboards.rookAttacks[enemyIdx]
        | bitboards.queenAttacks[enemyIdx];

    // Penalize concrete loss of tempo risk, not merely leaving the home square.
    return undevelopedMinors >= 3 && getBit(enemyAttacks, square);
}
}

std::pair<int, int> QueenEvaluation::evaluatePair(const Board& /*board*/, const Bitboards& bitboards, ChessColor color)
{
    constexpr int MaterialMG = QueenMaterialMG, MaterialEG = QueenMaterialEG;
    constexpr int MobBonusPerSquareMG = QueenMobilityPerSquareMG, MobBonusPerSquareEG = QueenMobilityPerSquareEG;
    constexpr int MobMaxMG = QueenMobilityCapMG, MobMaxEG = QueenMobilityCapEG;
    constexpr int CenterBonusMG = QueenCenterBonusMG, CenterBonusEG = QueenCenterBonusEG;
    constexpr int MobilityFloorMG = QueenMobilityOffsetMG, MobilityFloorEG = QueenMobilityOffsetEG;
    constexpr int VulnerableToMinorOrPawnPenaltyMG = 108, VulnerableToMinorOrPawnPenaltyEG = 116;
    constexpr int EarlyQueenPenaltyMG = ::EarlyQueenPenaltyMG, EarlyQueenPenaltyEG = ::EarlyQueenPenaltyEG;

    const int idx = Bitboards::indexOf(color);
    int mgScore = 0;
    int egScore = 0;

    Bitboard queens = bitboards.queens[idx];
    while (queens)
    {
        const Square square = popLeastSignificantBit(queens);

        mgScore += MaterialMG;
        egScore += MaterialEG;

        mgScore += CenterBonusMG;
        egScore += CenterBonusEG;

        // Early development is a problem only when the queen is actually
        // exposed to attack while the minor pieces still block development.
        if (queenIsExposedEarly(bitboards, color, square))
        {
            mgScore -= EarlyQueenPenaltyMG;
            egScore -= EarlyQueenPenaltyEG;
        }

        const int mobility = countQueenMobility(bitboards, color, square);
        mgScore += std::max(0, std::min(mobility * MobBonusPerSquareMG, MobMaxMG) - MobilityFloorMG);
        egScore += std::max(0, std::min(mobility * MobBonusPerSquareEG, MobMaxEG) - MobilityFloorEG);

        const ChessColor enemy = color == ChessColor::White ? ChessColor::Black : ChessColor::White;
        const Bitboard enemyMinorPawn = bitboards.pawnAttacks[Bitboards::indexOf(enemy)] |
            bitboards.knightAttacks[Bitboards::indexOf(enemy)] |
            bitboards.bishopAttacks[Bitboards::indexOf(enemy)];
        if (getBit(enemyMinorPawn, square))
        {
            mgScore -= VulnerableToMinorOrPawnPenaltyMG;
            egScore -= VulnerableToMinorOrPawnPenaltyEG;
        }
    }

    return {mgScore, egScore};
}

int QueenEvaluation::evaluate(const Board& board, const Bitboards& bitboards, ChessColor color)
{
    const auto [mg, eg] = evaluatePair(board, bitboards, color);
    const int phase = Evaluation::gamePhase(bitboards);
    return (mg * phase + eg * (24 - phase)) / 24;
}
