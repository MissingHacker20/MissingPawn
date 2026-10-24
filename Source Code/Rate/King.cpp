/*
 * MissingPawn
 * Copyright (C) 2026 Kacper Wieczorek
 *
 * This file is part of MissingPawn.
 *
 * Licensed under the GNU General Public License v3 or later.
 * See LICENSE for details.
 */

#include "Rate/King.h"
#include "Rate/Evaluation.h"

#include <algorithm>
#include <cmath>

#include "Foundation/Bitboard.h"
#include "Foundation/Bitboards.h"
#include "Foundation/Board.h"
#include "Move/AttackTables.h"


namespace
{
int countKingZoneAttacks(const Bitboards& bitboards, ChessColor color)
{
    const int idx = Bitboards::indexOf(color);
    Bitboard kingBB = bitboards.kings[idx];
    if (kingBB == 0) return 0;

    const Square kingSq = popLeastSignificantBit(kingBB);

    ChessColor enemy = (color == ChessColor::White) ? ChessColor::Black : ChessColor::White;

    const int enemyIdx = Bitboards::indexOf(enemy);
    const Bitboard zone = AttackTables::kingAttacks(kingSq) | (1ULL << static_cast<int>(kingSq));

    // Each attacked square is counted once per attacker type.  The weights are
    // the complete definition of this feature; there is no extra base count.
    return 2 * countBits(bitboards.pawnAttacks[enemyIdx] & zone)
         + 3 * countBits(bitboards.knightAttacks[enemyIdx] & zone)
         + 4 * countBits(bitboards.bishopAttacks[enemyIdx] & zone)
         + 5 * countBits(bitboards.rookAttacks[enemyIdx] & zone)
         + 6 * countBits(bitboards.queenAttacks[enemyIdx] & zone);
}

int countOpenFilesNearKing(const Bitboards& bitboards, ChessColor color)
{
    const int idx = Bitboards::indexOf(color);
    Bitboard kingBB = bitboards.kings[idx];
    if (kingBB == 0) return 0;

    Square kingSq = popLeastSignificantBit(kingBB);

    int kf = static_cast<int>(kingSq) % 8;
    int openCount = 0;

    constexpr Bitboard fileA = 0x0101010101010101ULL;
    const Bitboard ownPawns = bitboards.pawns[idx];

    for (int f = kf - 1; f <= kf + 1; ++f)
    {
        if (f < 0 || f >= 8) continue;
        if ((ownPawns & (fileA << f)) == 0)
        {
            ++openCount;
        }
    }
    return openCount;
}

}

std::pair<int, int> KingEvaluation::evaluatePair(const Board& board, const Bitboards& bitboards, ChessColor color)
{
    const Square kingSquare = board.getKingSquare(color);
    if (kingSquare == Square::None) return {0, 0};

    const int index = static_cast<int>(kingSquare);
    const int file = index % 8;
    const int rank = index / 8;

    const int idx = Bitboards::indexOf(color);
    const Bitboard ownPawns = bitboards.pawns[idx];

    // King safety uses separate MG/EG weights for the shield, pressure and files.
    constexpr int KingShieldCenterMG = 154;
    constexpr int KingShieldSideMG = 175;
    constexpr int KingShieldCenterEG = 66;
    constexpr int KingShieldSideEG = 0;
    constexpr int KingPressureBaseMG = 64;
    constexpr int KingPressureScaleMG = 4;
    constexpr int KingPressureCapMG = 35;
    constexpr int KingOpenFilePenaltyMG = 181;
    constexpr int KingPressureBaseEG = 3;
    constexpr int KingPressureScaleEG = 2;
    constexpr int KingPressureCapEG = KingPressureCapMG;
    constexpr int KingOpenFilePenaltyEG = 18;
    constexpr int KingNoDefendersPenaltyMG = 35;
    constexpr int KingNoDefendersPenaltyEG = 13;
    constexpr int KingActivityBaseEG = 252;
    constexpr int KingActivityDistanceWeightEG = 100;
    constexpr int KingPawnProximityWeightEG = 45;

    int mg = 0;
    int shield_mg = 0;
    int shield_eg = 0;
    const int shieldRank = rank + ((color == ChessColor::White) ? 1 : -1);
    if (shieldRank >= 0 && shieldRank < 8)
        for (int f = file - 1; f <= file + 1; ++f)
            if (f >= 0 && f < 8 && getBit(ownPawns, static_cast<Square>(shieldRank * 8 + f)))
            {
                shield_mg += (f == file) ? KingShieldCenterMG : KingShieldSideMG;
                shield_eg += (f == file) ? KingShieldCenterEG : KingShieldSideEG;
            }

    mg += shield_mg;

    const int pressure = countKingZoneAttacks(bitboards, color);
    const int pressurePenaltyMG = pressure * (KingPressureBaseMG
        + std::min(KingPressureCapMG, pressure * KingPressureScaleMG));
    const int openFilePenaltyMG = countOpenFilesNearKing(bitboards, color) * KingOpenFilePenaltyMG;
    const int pressurePenaltyEG = pressure * (KingPressureBaseEG
        + std::min(KingPressureCapEG, pressure * KingPressureScaleEG));
    const int openFilePenaltyEG = countOpenFilesNearKing(bitboards, color) * KingOpenFilePenaltyEG;
    mg -= pressurePenaltyMG;
    mg -= openFilePenaltyMG;
    int egSafety = -(pressurePenaltyEG + openFilePenaltyEG);

    Bitboard kingZone = AttackTables::kingAttacks(kingSquare) | (1ULL << index);
    const Bitboard defenders = bitboards.knights[idx] | bitboards.bishops[idx] |
                               bitboards.rooks[idx] | bitboards.queens[idx];
    if ((defenders & kingZone) == 0)
    {
        mg -= KingNoDefendersPenaltyMG;
        egSafety -= KingNoDefendersPenaltyEG;
    }

    // King activity (EG): centralisation plus proximity to the nearest own pawn.
    const int centerDist = std::abs(file * 2 - 7) + std::abs(rank * 2 - 7);
    int eg_activity = KingActivityBaseEG - centerDist * KingActivityDistanceWeightEG;
    int minDist = 14;
    Bitboard pawns = ownPawns;
    while (pawns)
    {
        const Square psq = popLeastSignificantBit(pawns);
        const int p = static_cast<int>(psq);
        minDist = std::min(minDist, std::abs(file - p % 8) + std::abs(rank - p / 8));
    }
    if (ownPawns) eg_activity += (14 - minDist) * KingPawnProximityWeightEG;
    int eg = eg_activity + shield_eg + egSafety;

    // Płynne taperowanie: MG King Safety × phase + EG King Activity × (1 - phase)
    // King safety naturalnie znika w końcówce, king activity rośnie.
    return {mg, eg};
}

int KingEvaluation::evaluate(const Board& board, const Bitboards& bitboards, ChessColor color)
{
    const auto [mg, eg] = evaluatePair(board, bitboards, color);
    const int phase = Evaluation::gamePhase(bitboards);
    return (mg * phase + eg * (24 - phase)) / 24;
}
