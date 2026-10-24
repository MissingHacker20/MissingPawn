/*
 * MissingPawn
 * Copyright (C) 2026 Kacper Wieczorek
 *
 * This file is part of MissingPawn.
 *
 * Licensed under the GNU General Public License v3 or later.
 * See LICENSE for details.
 */

#include "Rate/PawnStructure.h"
#include "Rate/Evaluation.h"

#include <algorithm>

#include "Foundation/Bitboard.h"
#include "Foundation/Bitboards.h"
#include "Foundation/Board.h"

namespace
{
constexpr int PieceBlockerMaxSquaresMG = 24;
constexpr int PieceBlockerMaxSquaresEG = 24;
constexpr int PieceBlockerWeightMG = 26;
constexpr int PieceBlockerWeightEG = 0;
constexpr int PieceSpaceWeightMG = 88;
constexpr int PieceSpaceWeightEG = 92;
constexpr int MaxGamePhase = 24;
constexpr Bitboard CenterFiles = 0x3C3C3C3C3C3C3C3CULL;
}

int PawnStructureEvaluation::PieceBlocker(const Board &board, const Bitboards &bitboards, ChessColor color)
{
    (void)board;
    const int enemyIdx = 1 - Bitboards::indexOf(color);
    const Bitboard blockedAttacks = bitboards.bishopBlockedSquares[enemyIdx] | bitboards.rookBlockedSquares[enemyIdx]
        | bitboards.queenBlockedSquares[enemyIdx] | bitboards.pawnBlockers[enemyIdx];
    return std::min(countBits(blockedAttacks), PieceBlockerMaxSquaresMG) * PieceBlockerWeightMG;
}

int PawnStructureEvaluation::PieceSpace(const Board & /*board*/, const Bitboards &bitboards, ChessColor color)
{
    const int ownIdx = Bitboards::indexOf(color);
    const int enemyIdx = 1 - ownIdx;
    const Bitboard ownAttacks = bitboards.pawnAttacks[ownIdx] | bitboards.knightAttacks[ownIdx]
        | bitboards.bishopAttacks[ownIdx] | bitboards.rookAttacks[ownIdx] | bitboards.queenAttacks[ownIdx];
    const Bitboard enemyAttacks = bitboards.pawnAttacks[enemyIdx] | bitboards.knightAttacks[enemyIdx]
        | bitboards.bishopAttacks[enemyIdx] | bitboards.rookAttacks[enemyIdx] | bitboards.queenAttacks[enemyIdx];
    Bitboard enemyHalf = 0;
    const int firstEnemyRank = color == ChessColor::White ? 4 : 0;
    const int lastEnemyRank = color == ChessColor::White ? 7 : 3;
    for (int rank = firstEnemyRank; rank <= lastEnemyRank; ++rank)
        enemyHalf |= 0xFFULL << (rank * 8);
    const int squares = countBits(ownAttacks & ~enemyAttacks & enemyHalf & CenterFiles);
    return squares * PieceSpaceWeightMG;
}

std::pair<int, int> PawnStructureEvaluation::evaluatePair(const Board& /*board*/, const Bitboards& bitboards, ChessColor color)
{
    const int enemyIdx = 1 - Bitboards::indexOf(color);
    const Bitboard blockedAttacks = bitboards.bishopBlockedSquares[enemyIdx]
        | bitboards.rookBlockedSquares[enemyIdx]
        | bitboards.queenBlockedSquares[enemyIdx]
        | bitboards.pawnBlockers[enemyIdx];
    const int blockerSquaresMG = std::min(countBits(blockedAttacks), PieceBlockerMaxSquaresMG);
    const int blockerSquaresEG = std::min(countBits(blockedAttacks), PieceBlockerMaxSquaresEG);
    const int blockerMG = blockerSquaresMG * PieceBlockerWeightMG;
    const int blockerEG = blockerSquaresEG * PieceBlockerWeightEG;

    const Bitboard ownAttacks = bitboards.pawnAttacks[Bitboards::indexOf(color)]
        | bitboards.knightAttacks[Bitboards::indexOf(color)]
        | bitboards.bishopAttacks[Bitboards::indexOf(color)]
        | bitboards.rookAttacks[Bitboards::indexOf(color)]
        | bitboards.queenAttacks[Bitboards::indexOf(color)];
    const Bitboard enemyAttacks = bitboards.pawnAttacks[enemyIdx]
        | bitboards.knightAttacks[enemyIdx]
        | bitboards.bishopAttacks[enemyIdx]
        | bitboards.rookAttacks[enemyIdx]
        | bitboards.queenAttacks[enemyIdx];
    Bitboard enemyHalf = 0;
    const int firstEnemyRank = color == ChessColor::White ? 4 : 0;
    const int lastEnemyRank = color == ChessColor::White ? 7 : 3;
    for (int rank = firstEnemyRank; rank <= lastEnemyRank; ++rank)
        enemyHalf |= 0xFFULL << (rank * 8);
    const int spaceSquares = countBits(ownAttacks & ~enemyAttacks & enemyHalf & CenterFiles);
    const int spaceMG = spaceSquares * PieceSpaceWeightMG;
    const int spaceEG = spaceSquares * PieceSpaceWeightEG;

    return {blockerMG + spaceMG, blockerEG + spaceEG};
}

int PawnStructureEvaluation::evaluate(const Board& board, const Bitboards& bitboards, ChessColor color)
{
    const auto [mg, eg] = evaluatePair(board, bitboards, color);
    const int phase = Evaluation::gamePhase(bitboards);
    return (mg * phase + eg * (MaxGamePhase - phase)) / MaxGamePhase;
}

Bitboard PawnStructureEvaluation::getAffectedPawns(const Board &board, const Move &move)
{
    (void)board;
    (void)move;
    return 0;
}
