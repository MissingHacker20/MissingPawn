/*
 * MissingPawn
 * Copyright (C) 2026 Kacper Wieczorek
 *
 * This file is part of MissingPawn.
 *
 * Licensed under the GNU General Public License v3 or later.
 * See LICENSE for details.
 */

#include "Rate/Pawn.h"
#include "Rate/Evaluation.h"

#include <algorithm>
#include <array>
#include <cmath>

#include "Foundation/Bitboard.h"
#include "Foundation/Bitboards.h"

namespace
{
constexpr Bitboard fileA = 0x0101010101010101ULL;

inline Bitboard fileMaskBB(int file)
{
    return fileA << file;
}

inline int pawnProgress(ChessColor color, Square square)
{
    const int rank = static_cast<int>(square) / 8;
    return color == ChessColor::White ? rank : 7 - rank;
}

inline int fileOf(Square square)
{
    return static_cast<int>(square) % 8;
}

inline int rankOf(Square square)
{
    return static_cast<int>(square) / 8;
}

// Maski pól przed pionkiem (w kierunku promocji) na liniach file-1..file+1
inline Bitboard forwardMask(ChessColor color, int file, int rank)
{
    Bitboard mask = 0;
    const int step = (color == ChessColor::White) ? 1 : -1;

    for (int r = rank + step; r >= 0 && r < 8; r += step)
    {
        if (file > 0)     mask |= 1ULL << (r * 8 + file - 1);
        mask |= 1ULL << (r * 8 + file);
        if (file < 7)     mask |= 1ULL << (r * 8 + file + 1);
    }

    return mask;
}

// Pionki przeciwnika przed podanym pionkiem (na liniach bocznych i własnej)
inline Bitboard enemyPawnsAhead(const Bitboards& bitboards, ChessColor color, Square square)
{
    const int file = fileOf(square);
    const int rank = rankOf(square);
    return bitboards.pawns[1 - Bitboards::indexOf(color)] & forwardMask(color, file, rank);
}

bool isPassedPawn(const Bitboards& bitboards, ChessColor color, Square square)
{
    return enemyPawnsAhead(bitboards, color, square) == 0;
}

bool isCandidatePassedPawn(const Bitboards& bitboards, ChessColor color, Square square)
{
    // Kandydat musi mieć konkretną drogę do przełamania: liczymy wyłącznie
    // piony przeciwnika przed nim oraz własne piony na sąsiednich liniach,
    // które mogą wesprzeć marsz lub wymianę pionową. Sama liczba pionów na
    // trzech kolumnach nie jest wystarczającym warunkiem.
    if (isPassedPawn(bitboards, color, square)) return false;

    const int idx = Bitboards::indexOf(color);
    const int file = fileOf(square);
    const int rank = rankOf(square);
    const int step = color == ChessColor::White ? 1 : -1;
    const Bitboard ownPawns = bitboards.pawns[idx];
    const Bitboard enemyPawns = bitboards.pawns[1 - idx];

    int enemyBlockers = 0;
    int usableHelpers = 0;
    for (int f = std::max(0, file - 1); f <= std::min(7, file + 1); ++f)
    {
        for (int r = rank + step; r >= 0 && r < 8; r += step)
        {
            if (getBit(enemyPawns, static_cast<Square>(r * 8 + f)))
            {
                ++enemyBlockers;
                break;
            }
        }

        if (f == file) continue;
        for (int r = rank - step; r >= 0 && r < 8; r -= step)
        {
            if (getBit(ownPawns, static_cast<Square>(r * 8 + f)))
            {
                ++usableHelpers;
                break;
            }
        }
    }

    return enemyBlockers > 0 && usableHelpers >= enemyBlockers;
}

bool isPawnProtected(const Bitboards& bitboards, ChessColor color, Square square)
{
    const int idx = Bitboards::indexOf(color);
    const int file = fileOf(square);
    const int rank = rankOf(square);
    const int protectRank = rank + ((color == ChessColor::White) ? -1 : 1);

    if (protectRank < 0 || protectRank >= 8) return false;

    for (int f = file - 1; f <= file + 1; f += 2)
    {
        if (f >= 0 && f < 8 &&
            getBit(bitboards.pawns[idx], static_cast<Square>(protectRank * 8 + f)))
        {
            return true;
        }
    }
    return false;
}

bool hasNeighborPawn(const Bitboards& bitboards, ChessColor color, int file, int rank)
{
    const Bitboard ownPawns = bitboards.pawns[Bitboards::indexOf(color)];

    for (int f = file - 1; f <= file + 1; f += 2)
    {
        if (f >= 0 && f < 8)
        {
            for (int dr = -1; dr <= 1; ++dr)
            {
                const int r = rank + dr;
                if (r >= 0 && r < 8 &&
                    getBit(ownPawns, static_cast<Square>(r * 8 + f)))
                {
                    return true;
                }
            }
        }
    }
    return false;
}

bool isPhalanxPawn(const Bitboards& bitboards, ChessColor color, int file, int rank)
{
    const Bitboard ownPawns = bitboards.pawns[Bitboards::indexOf(color)];

    for (int f = file - 1; f <= file + 1; f += 2)
    {
        if (f >= 0 && f < 8 &&
            getBit(ownPawns, static_cast<Square>(rank * 8 + f)))
        {
            return true;
        }
    }
    return false;
}

bool isBackwardPawn(const Bitboards& bitboards, ChessColor color, Square square)
{
    const int idx = Bitboards::indexOf(color);
    const int file = fileOf(square);
    const int rank = rankOf(square);
    const int step = (color == ChessColor::White) ? 1 : -1;

    const Bitboard ownPawns = bitboards.pawns[idx];
    const Bitboard enemyPawns = bitboards.pawns[1 - idx];

    // Brak własnych pionków "wstecz" na liniach bocznych
    for (int f = file - 1; f <= file + 1; f += 2)
    {
        if (f < 0 || f > 7) continue;
        for (int r = rank; r >= 0 && r < 8; r -= step)
        {
            if (getBit(ownPawns, static_cast<Square>(r * 8 + f)))
            {
                return false;
            }
        }
    }

    // Pionek przeciwnika kontroluje pole stopu
    const int stopRank = rank + step;
    if (stopRank >= 0 && stopRank < 8)
    {
        for (int f = file - 1; f <= file + 1; f += 2)
        {
            if (f >= 0 && f < 8 &&
                getBit(enemyPawns, static_cast<Square>(stopRank * 8 + f)))
            {
                return true;
            }
        }
    }

    return false;
}

bool isCentralPawn(Square square)
{
    const int file = fileOf(square);
    const int rank = rankOf(square);
    return (file >= 2 && file <= 5) && (rank >= 2 && rank <= 5);
}

bool isLockedPawn(const Bitboards& bitboards, ChessColor color, Square square)
{
    const int file = fileOf(square);
    const int rank = rankOf(square);
    const int step = (color == ChessColor::White) ? 1 : -1;
    const int frontRank = rank + step;

    if (frontRank >= 0 && frontRank < 8)
    {
        return getBit(bitboards.pawns[1 - Bitboards::indexOf(color)],
                      static_cast<Square>(frontRank * 8 + file));
    }
    return false;
}

bool isPawnLever(const Bitboards& bitboards, ChessColor color, Square square)
{
    const int file = fileOf(square);
    const int rank = rankOf(square);
    const int step = (color == ChessColor::White) ? 1 : -1;
    const int targetRank = rank + step;

    if (targetRank < 0 || targetRank >= 8) return false;

    for (int f = file - 1; f <= file + 1; f += 2)
    {
        if (f >= 0 && f < 8 &&
            getBit(bitboards.pawns[1 - Bitboards::indexOf(color)],
                   static_cast<Square>(targetRank * 8 + f)))
        {
            return true;
        }
    }
    return false;
}

bool isIsolatedPawnOnFile(const Bitboards& bitboards, ChessColor color, int file)
{
    // Izolacja pionków przeciwnika (używane w leverOpportunityScore)
    const int enemyIdx = 1 - Bitboards::indexOf(color);
    const Bitboard enemyPawns = bitboards.pawns[enemyIdx];

    const bool leftEmpty = file == 0 || (enemyPawns & fileMaskBB(file - 1)) == 0;
    const bool rightEmpty = file == 7 || (enemyPawns & fileMaskBB(file + 1)) == 0;

    return leftEmpty && rightEmpty && (enemyPawns & fileMaskBB(file)) != 0;
}

int leverOpportunityScore(const Bitboards& bitboards, ChessColor color, Square square)
{
    const int file = fileOf(square);
    const int rank = rankOf(square);
    const int step = (color == ChessColor::White) ? 1 : -1;
    const int targetRank = rank + step;

    if (targetRank < 0 || targetRank >= 8) return 0;

    const int idx = Bitboards::indexOf(color);
    const Bitboard enemyPawns = bitboards.pawns[1 - idx];

    int score = 0;

    for (int f = file - 1; f <= file + 1; f += 2)
    {
        if (f < 0 || f > 7) continue;

        const Square leverSq = static_cast<Square>(targetRank * 8 + f);

        if (getBit(enemyPawns, leverSq))
        {
            const int enemyProgress = pawnProgress(color, leverSq);
            score += 60 + enemyProgress * 20;

            if (isBackwardPawn(bitboards, color, leverSq))
            {
                score += 80;
            }

            if (isIsolatedPawnOnFile(bitboards, color, f))
            {
                score += 50;
            }
        }
    }

    return score;
}

bool isIsolatedPawn(const int* fileCounts, int file)
{
    const bool leftEmpty = file == 0 || fileCounts[file - 1] == 0;
    const bool rightEmpty = file == 7 || fileCounts[file + 1] == 0;
    return leftEmpty && rightEmpty;
}

int countPawnIslands(const int* fileCounts)
{
    int islands = 0;
    bool inIsland = false;
    for (int f = 0; f < 8; ++f)
    {
        if (fileCounts[f] > 0)
        {
            if (!inIsland)
            {
                islands++;
                inIsland = true;
            }
        }
        else
        {
            inIsland = false;
        }
    }
    return islands;
}

int countPawnMajority(const Bitboards& bitboards, ChessColor color)
{
    const int own = Bitboards::indexOf(color);
    const int enemy = 1 - own;
    int bestMajority = 0;

    for (int firstFile : {0, 4})
    {
        int ownCount = 0;
        int enemyCount = 0;
        for (int file = firstFile; file < firstFile + 4; ++file)
        {
            ownCount += countBits(bitboards.pawns[own] & fileMaskBB(file));
            enemyCount += countBits(bitboards.pawns[enemy] & fileMaskBB(file));
        }
        bestMajority = std::max(bestMajority, ownCount - enemyCount);
    }
    return std::max(0, bestMajority);
}

bool hasConnectedPassedPawn(const Bitboards& bitboards, ChessColor color, Square square)
{
    if (!isPassedPawn(bitboards, color, square)) return false;

    const int file = fileOf(square);
    const int rank = rankOf(square);
    const int own = Bitboards::indexOf(color);
    // Liczymy parę tylko od jej lewej strony, aby nie nagradzać jej dwa razy.
    for (int adjacentFile : {file + 1})
    {
        if (adjacentFile < 0 || adjacentFile > 7) continue;
        for (int adjacentRank = rank - 1; adjacentRank <= rank + 1; ++adjacentRank)
        {
            if (adjacentRank >= 0 && adjacentRank < 8 &&
                getBit(bitboards.pawns[own], static_cast<Square>(adjacentRank * 8 + adjacentFile)) &&
                isPassedPawn(bitboards, color,
                    static_cast<Square>(adjacentRank * 8 + adjacentFile)))
            {
                return true;
            }
        }
    }
    return false;
}

} // namespace

std::pair<int, int> PawnEvaluation::evaluatePair(const Board& /*board*/, const Bitboards& bitboards, ChessColor color)
{
    constexpr int PassedBonusBaseMG = 164;
    constexpr int PassedBonusBaseEG = 244;
    constexpr int CandidatePassedBonusBaseMG = 0;
    constexpr int CandidatePassedBonusBaseEG = 5;
    constexpr int PawnMaterialMG = 992;
    constexpr int PawnMaterialEG = 1135;
    constexpr int PassedProgressBonusMG[4] = {191, 360, 559, 750};
    constexpr int PassedProgressBonusEG[4] = {228, 406, 638, 820};
    constexpr int PawnProgressPerRankMG = 83;
    constexpr int PawnProgressPerRankEG = 89;
    constexpr int CandidatePassedProgressBaseMG = 8;
    constexpr int CandidatePassedProgressBaseEG = 0;
    constexpr int MultiplePassedBaseMG = 3;
    constexpr int MultiplePassedBaseEG = 58;
    constexpr int PassedPawnLockedPenaltyMG = 120;
    constexpr int PassedPawnLockedPenaltyEG = 150;
    constexpr int PassedPawnBlockedPenaltyMG = 89;
    constexpr int PassedPawnBlockedPenaltyEG = 122;
    constexpr int ProtectedPassedBonusMG = 100;
    constexpr int ProtectedPassedBonusEG = 129;
    constexpr int ConnectedBonusMG = 43;
    constexpr int ConnectedBonusEG = 0;
    constexpr int LeverOpportunityWeightMG = 12;
    constexpr int LeverOpportunityWeightEG = 0;
    constexpr int PhalanxBonusMG = 131;
    constexpr int PhalanxBonusEG = 5;
    constexpr int ProtectedBonusMG = 108;
    constexpr int ProtectedBonusEG = 0;
    constexpr int IsolatedPenaltyMG = 80;
    constexpr int IsolatedPenaltyEG = 0;
    constexpr int DoubledPenaltyMG = 32;
    constexpr int DoubledPenaltyEG = 191;
    constexpr int TripledPenaltyMG = 190;
    constexpr int TripledPenaltyEG = 193;
    constexpr int BackwardPenaltyMG = 89;
    constexpr int BackwardPenaltyEG = 0;
    constexpr int IslandPenaltyMG = 17;
    constexpr int IslandPenaltyEG = 0;
    constexpr int MajorityBonusMG = 89;
    constexpr int MajorityBonusEG = 144;
    constexpr int LeverBonusMG = 5;
    constexpr int LeverBonusEG = 9;
    constexpr int CentralBonusMG = 16;
    constexpr int CentralBonusEG = 0;
    constexpr int ConnectedPassedBonusMG = 141;
    constexpr int ConnectedPassedBonusEG = 186;
    constexpr int MutualDefenseBonusMG = 16;
    constexpr int MutualDefenseBonusEG = 22;

    const int idx = Bitboards::indexOf(color);
    int mgScore = 0;
    int egScore = 0;
    std::array<int, 8> pawnsOnFile{};
    int passedCount = 0;
    int connectedPassedCount = 0;

    Bitboard pawns = bitboards.pawns[idx];
    while (pawns)
    {
        const Square square = popLeastSignificantBit(pawns);

        const int file = fileOf(square);
        const int rank = rankOf(square);
        const int progress = pawnProgress(color, square);

        pawnsOnFile[file]++;
        const bool protectedPawn = isPawnProtected(bitboards, color, square);
        const bool passedPawn   = isPassedPawn(bitboards, color, square);

        // Materiał pionka jest stały; zaawansowanie samo w sobie ma tylko
        // niewielką wartość i nie może być doliczane ponownie jako passed pawn.
        mgScore += PawnMaterialMG + progress * PawnProgressPerRankMG;
        egScore += PawnMaterialEG + progress * PawnProgressPerRankEG;

        if (isCentralPawn(square)) { mgScore += CentralBonusMG; egScore += CentralBonusEG; }
        if (protectedPawn) { mgScore += ProtectedBonusMG; egScore += ProtectedBonusEG; }
        // Phalanx i connected opisują w dużej mierze ten sam układ. Premia
        // jest celowo mała, aby nie sumować kilku nazw dla jednego faktu.
        if (isPhalanxPawn(bitboards, color, file, rank)) { mgScore += PhalanxBonusMG / 2; egScore += PhalanxBonusEG / 2; }
        else if (hasNeighborPawn(bitboards, color, file, rank)) { mgScore += ConnectedBonusMG / 2; egScore += ConnectedBonusEG / 2; }

        if (passedPawn)
        {
            ++passedCount;
            const int progressIndex = std::min(progress, 7);
            mgScore += PassedBonusBaseMG;
            egScore += PassedBonusBaseEG;
            if (progressIndex >= 4)
            {
                mgScore += PassedProgressBonusMG[progressIndex - 4];
                egScore += PassedProgressBonusEG[progressIndex - 4];
            }
            // Zablokowany wolny pion jest znacznie mniej praktycznie wart.
            if (isLockedPawn(bitboards, color, square)) { mgScore -= PassedPawnLockedPenaltyMG; egScore -= PassedPawnLockedPenaltyEG; }
            // A blocker or an unsafe promotion route reduces practical value.
            const int forward = color == ChessColor::White ? 1 : -1;
            const int nextRank = rank + forward;
            if (nextRank >= 0 && nextRank < 8 &&
                getBit(bitboards.allOccupied, static_cast<Square>(nextRank * 8 + file)))
            {
                mgScore -= PassedPawnBlockedPenaltyMG;
                egScore -= PassedPawnBlockedPenaltyEG;
            }
            if (protectedPawn)
            {
                mgScore += ProtectedPassedBonusMG;
                egScore += ProtectedPassedBonusEG;
            }
        }

        if (isCandidatePassedPawn(bitboards, color, square))
        {
            mgScore += CandidatePassedBonusBaseMG + std::max(0, progress - 2) * CandidatePassedProgressBaseMG;
            egScore += CandidatePassedBonusBaseEG + std::max(0, progress - 2) * CandidatePassedProgressBaseEG;
        }

        if (isPawnLever(bitboards, color, square)) { mgScore += LeverBonusMG; egScore += LeverBonusEG; }
        const int leverOpp = leverOpportunityScore(bitboards, color, square);
        if (leverOpp > 0)
        {
            mgScore += leverOpp * LeverOpportunityWeightMG;
            egScore += leverOpp * LeverOpportunityWeightEG;
        }

        if (isBackwardPawn(bitboards, color, square)) { mgScore -= BackwardPenaltyMG; egScore -= BackwardPenaltyEG; }

        if (hasConnectedPassedPawn(bitboards, color, square))
        {
            ++connectedPassedCount;
        }
    }

    for (int file = 0; file < 8; ++file)
    {
        if (pawnsOnFile[file] > 0 && isIsolatedPawn(pawnsOnFile.data(), file))
        {
            mgScore -= IsolatedPenaltyMG * pawnsOnFile[file];
            egScore -= IsolatedPenaltyEG * pawnsOnFile[file];
        }

        if (pawnsOnFile[file] > 1)
        {
            mgScore -= DoubledPenaltyMG * (pawnsOnFile[file] - 1);
            egScore -= DoubledPenaltyEG * (pawnsOnFile[file] - 1);
        }

        if (pawnsOnFile[file] >= 3)
        {
            mgScore -= TripledPenaltyMG;
            egScore -= TripledPenaltyEG;
        }
    }

    if (passedCount > 1)
    {
        mgScore += passedCount * MultiplePassedBaseMG;
        egScore += passedCount * MultiplePassedBaseEG;
    }
    mgScore += connectedPassedCount * (ConnectedPassedBonusMG / 2);
    egScore += connectedPassedCount * (ConnectedPassedBonusEG / 2);
    const int majority = countPawnMajority(bitboards, color);
    if (majority > 0)
    {
        mgScore += MajorityBonusMG * std::min(3, majority);
        egScore += MajorityBonusEG * std::min(3, majority);
    }

    const int islands = countPawnIslands(pawnsOnFile.data());
    if (islands > 1)
    {
        mgScore -= (islands - 1) * IslandPenaltyMG;
        egScore -= (islands - 1) * IslandPenaltyEG;
    }

    // Ochrona i połączenie są już uwzględnione przy konkretnych pionach;
    // nie doliczamy drugiego bonusu za ten sam fakt strukturalny.
    if (majority > 1) { mgScore += MutualDefenseBonusMG / 2; egScore += MutualDefenseBonusEG / 2; }

    return {mgScore, egScore};
}

int PawnEvaluation::evaluate(const Board& board, const Bitboards& bitboards, ChessColor color)
{
    const auto [mg, eg] = evaluatePair(board, bitboards, color);
    const int phase = Evaluation::gamePhase(bitboards);
    return (mg * phase + eg * (24 - phase)) / 24;
}
