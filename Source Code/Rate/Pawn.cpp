#include "Rate/Pawn.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

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
    if (isPassedPawn(bitboards, color, square)) return false;

    const int idx = Bitboards::indexOf(color);
    const int file = fileOf(square);
    const int rank = rankOf(square);
    const int step = (color == ChessColor::White) ? 1 : -1;

    const Bitboard ownPawns = bitboards.pawns[idx];
    const Bitboard enemyPawns = bitboards.pawns[1 - idx];

    int enemyBlockers = 0;
    int friendlyHelpers = 0;

    for (int f = std::max(0, file - 1); f <= std::min(7, file + 1); ++f)
    {
        // Blokujący pionek/przedmiot przeciwnika przed pionkiem na linii f
        bool hasEnemyBlocker = false;
        for (int r = rank + step; r >= 0 && r < 8; r += step)
        {
            if (getBit(enemyPawns, static_cast<Square>(r * 8 + f)) ||
                getBit(bitboards.occupied[1 - idx] & ~enemyPawns, static_cast<Square>(r * 8 + f)))
            {
                hasEnemyBlocker = true;
                break;
            }
        }

        if (hasEnemyBlocker) enemyBlockers++;

        if (f == file) friendlyHelpers++;

        friendlyHelpers += countBits(ownPawns & fileMaskBB(f));
    }

    return friendlyHelpers > enemyBlockers;
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

bool hasSameFileNeighbor(const Bitboards& bitboards, ChessColor color, int file, int rank)
{
    const Bitboard ownPawns = bitboards.pawns[Bitboards::indexOf(color)];

    for (int dr = -1; dr <= 1; dr += 2)
    {
        const int r = rank + dr;
        if (r >= 0 && r < 8 &&
            getBit(ownPawns, static_cast<Square>(r * 8 + file)))
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
            score += 6 + enemyProgress * 2;

            if (isBackwardPawn(bitboards, color, leverSq))
            {
                score += 8;
            }

            if (isIsolatedPawnOnFile(bitboards, color, f))
            {
                score += 5;
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

int countPawnMajority(const std::vector<int>& files)
{
    int majority = 0;
    for (int file : files)
    {
        if (file > 1) majority++;
    }
    return majority;
}

int countSpaceControl(ChessColor color, Square square)
{
    const int file = fileOf(square);
    const int rank = rankOf(square);
    const int enemyHalf = color == ChessColor::White ? 4 : 3;
    int control = 0;

    for (int f = std::max(0, file - 1); f <= std::min(7, file + 1); ++f)
    {
        for (int r = std::max(0, rank - 1); r <= std::min(7, rank + 1); ++r)
        {
            const int distance = std::abs(f - file) + std::abs(r - rank);
            if (distance <= 1) continue;

            if (color == ChessColor::White && r >= enemyHalf) control++;
            else if (color == ChessColor::Black && r <= enemyHalf) control++;
        }
    }

    return control;
}

void computeActiveFiles(const Bitboards& bitboards, ChessColor color, bool activeFiles[8])
{
    std::fill(activeFiles, activeFiles + 8, false);

    Bitboard pawns = bitboards.pawns[Bitboards::indexOf(color)];
    while (pawns)
    {
        const Square square = popLeastSignificantBit(pawns);
        const int file = fileOf(square);
        activeFiles[file] = true;
        if (file > 0) activeFiles[file - 1] = true;
        if (file < 7) activeFiles[file + 1] = true;
    }
}

}

int PawnEvaluation::evaluate(const Board& /*board*/, const Bitboards& bitboards, ChessColor color)
{
    constexpr int PassedBonusBase = 30;
    constexpr int CandidatePassedBonusBase = 12;
    constexpr int ProtectedPassedBonus = 30;
    constexpr int ConnectedBonus = 12;
    constexpr int PawnChainBonus = 9;
    constexpr int PhalanxBonus = 15;
    constexpr int ProtectedBonus = 8;
    constexpr int IsolatedPenalty = 15;
    constexpr int DoubledPenalty = 18;
    constexpr int TripledPenalty = 38;
    constexpr int BackwardPenalty = 18;
    constexpr int IslandPenalty = 9;
    constexpr int MajorityBonus = 12;
    constexpr int SpaceControlBonus = 5;
    constexpr int LockedBonus = 8;
    constexpr int LeverBonus = 6;
    constexpr int CentralBonus = 12;
    constexpr int PassedSupportBonus = 15;
    constexpr int WeakSquareBonus = 9;
    constexpr int ConnectedPassedBonus = 45;
    constexpr int MutualDefenseBonus = 8;

    const int idx = Bitboards::indexOf(color);
    int score = 0;
    std::array<int, 8> pawnsOnFile{};
    std::vector<int> pawnFileCounts;
    int passedCount = 0;
    int connectedPassedCount = 0;
    int protectedPassedCount = 0;

    bool activeFiles[8] = {};
    computeActiveFiles(bitboards, color, activeFiles);

    Bitboard pawns = bitboards.pawns[idx];
    while (pawns)
    {
        const Square square = popLeastSignificantBit(pawns);

        const int file = fileOf(square);
        if (!activeFiles[file]) continue;

        const int rank = rankOf(square);
        const int progress = pawnProgress(color, square);

        pawnsOnFile[file]++;
        pawnFileCounts.push_back(file);

        const bool protectedPawn = isPawnProtected(bitboards, color, square);
        const bool passedPawn   = isPassedPawn(bitboards, color, square);

        score += 150 + progress * 9;

        if (isCentralPawn(square)) score += CentralBonus;
        if (protectedPawn) score += ProtectedBonus;
        if (isPhalanxPawn(bitboards, color, file, rank)) score += PhalanxBonus;
        if (hasNeighborPawn(bitboards, color, file, rank)) score += ConnectedBonus;

        if (passedPawn)
        {
            ++passedCount;
            score += PassedBonusBase + progress * 15;
            if (protectedPawn)
            {
                ++protectedPassedCount;
                score += ProtectedPassedBonus;
            }
        }

        if (isCandidatePassedPawn(bitboards, color, square))
        {
            score += CandidatePassedBonusBase + std::max(0, progress - 2) * 6;
        }

        if (isPawnLever(bitboards, color, square)) score += LeverBonus;
        if (isLockedPawn(bitboards, color, square)) score += LockedBonus;
        if (hasSameFileNeighbor(bitboards, color, file, rank)) score += PawnChainBonus;

        const int leverOpp = leverOpportunityScore(bitboards, color, square);
        if (leverOpp > 0)
        {
            score += leverOpp;
        }

        const int spaceControl = countSpaceControl(color, square);
        if (spaceControl > 0)
        {
            score += SpaceControlBonus + std::min(8, spaceControl);
        }

        if (isBackwardPawn(bitboards, color, square)) score -= BackwardPenalty;

        if (passedPawn && protectedPawn)
        {
            ++connectedPassedCount;
            score += ConnectedPassedBonus;
        }
    }

    for (int file = 0; file < 8; ++file)
    {
        if (pawnsOnFile[file] > 0 && isIsolatedPawn(pawnsOnFile.data(), file))
        {
            score -= IsolatedPenalty * pawnsOnFile[file];
        }

        if (pawnsOnFile[file] > 1)
        {
            score -= DoubledPenalty * (pawnsOnFile[file] - 1);
        }

        if (pawnsOnFile[file] >= 3)
        {
            score -= TripledPenalty;
        }
    }

    if (passedCount > 1) score += passedCount * 9;
    if (protectedPassedCount > 1) score += protectedPassedCount * 6;
    if (connectedPassedCount > 1) score += connectedPassedCount * 12;

    if (countPawnMajority(pawnFileCounts) > 0)
    {
        score += MajorityBonus * std::min(3, countPawnMajority(pawnFileCounts));
    }

    const int islands = countPawnIslands(pawnsOnFile.data());
    if (islands > 1) score -= (islands - 1) * IslandPenalty;

    if (protectedPassedCount > 0) score += PassedSupportBonus * protectedPassedCount;
    if (connectedPassedCount > 0) score += WeakSquareBonus * connectedPassedCount;
    if (countPawnMajority(pawnFileCounts) > 1) score += MutualDefenseBonus;

    return score;
}
