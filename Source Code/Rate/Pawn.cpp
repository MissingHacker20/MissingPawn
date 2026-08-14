#include "Rate/Pawn.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace
{
inline Piece pawnFor(ChessColor color)
{
    return color == ChessColor::White ? Piece::WhitePawn : Piece::BlackPawn;
}

inline Piece enemyPawnFor(ChessColor color)
{
    return color == ChessColor::White ? Piece::BlackPawn : Piece::WhitePawn;
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

bool isPassedPawn(const Board& board, ChessColor color, Square square)
{
    const int file = fileOf(square);
    const int rank = rankOf(square);
    const int step = color == ChessColor::White ? 1 : -1;
    const Piece enemyPawn = enemyPawnFor(color);

    for (int targetRank = rank + step; targetRank >= 0 && targetRank < 8; targetRank += step)
    {
        for (int targetFile = file - 1; targetFile <= file + 1; ++targetFile)
        {
            if (targetFile >= 0 && targetFile < 8)
            {
                if (board.pieceAt(static_cast<Square>(targetRank * 8 + targetFile)) == enemyPawn)
                {
                    return false;
                }
            }
        }
    }
    return true;
}

bool isCandidatePassedPawn(const Board& board, ChessColor color, Square square)
{
    if (isPassedPawn(board, color, square)) return false;

    const int file = fileOf(square);
    const int rank = rankOf(square);
    const int step = color == ChessColor::White ? 1 : -1;
    const Piece enemyPawn = enemyPawnFor(color);
    const Piece ownPawn = pawnFor(color);

    int enemyBlockers = 0;
    int friendlyHelpers = 0;

    for (int targetFile = file - 1; targetFile <= file + 1; ++targetFile)
    {
        if (targetFile < 0 || targetFile > 7) continue;

        bool hasEnemyBlocker = false;
        for (int targetRank = rank + step; targetRank >= 0 && targetRank < 8; targetRank += step)
        {
            const Square sq = static_cast<Square>(targetRank * 8 + targetFile);
            const Piece p = board.pieceAt(sq);
            if (p == enemyPawn)
            {
                hasEnemyBlocker = true;
                break;
            }
            if (p != Piece::None && p != ownPawn)
            {
                hasEnemyBlocker = true;
                break;
            }
        }

        if (hasEnemyBlocker) enemyBlockers++;

        if (targetFile == file) friendlyHelpers++;

        for (int targetRank = 0; targetRank < 8; ++targetRank)
        {
            const Square sq = static_cast<Square>(targetRank * 8 + targetFile);
            if (board.pieceAt(sq) == ownPawn) friendlyHelpers++;
        }
    }

    return friendlyHelpers > enemyBlockers;
}

bool isPawnProtected(const Board& board, ChessColor color, Square square)
{
    const Piece ownPawn = pawnFor(color);
    const int file = fileOf(square);
    const int rank = rankOf(square);
    const int protectRank = rank + ((color == ChessColor::White) ? -1 : 1);

    if (protectRank < 0 || protectRank >= 8) return false;

    for (int f = file - 1; f <= file + 1; f += 2)
    {
        if (f >= 0 && f < 8 && board.pieceAt(static_cast<Square>(protectRank * 8 + f)) == ownPawn)
        {
            return true;
        }
    }
    return false;
}

bool hasNeighborPawn(const Board& board, Piece pawn, int file, int rank)
{
    for (int f = file - 1; f <= file + 1; f += 2)
    {
        if (f >= 0 && f < 8)
        {
            for (int dr = -1; dr <= 1; ++dr)
            {
                const int r = rank + dr;
                if (r >= 0 && r < 8 && board.pieceAt(static_cast<Square>(r * 8 + f)) == pawn)
                {
                    return true;
                }
            }
        }
    }
    return false;
}

bool isPhalanxPawn(const Board& board, Piece pawn, int file, int rank)
{
    for (int f = file - 1; f <= file + 1; f += 2)
    {
        if (f >= 0 && f < 8 && board.pieceAt(static_cast<Square>(rank * 8 + f)) == pawn)
        {
            return true;
        }
    }
    return false;
}

bool hasSameFileNeighbor(const Board& board, Piece pawn, int file, int rank)
{
    for (int dr = -1; dr <= 1; dr += 2)
    {
        const int r = rank + dr;
        if (r >= 0 && r < 8 && board.pieceAt(static_cast<Square>(r * 8 + file)) == pawn)
        {
            return true;
        }
    }
    return false;
}

bool isBackwardPawn(const Board& board, ChessColor color, Square square)
{
    const int file = fileOf(square);
    const int rank = rankOf(square);
    const int step = color == ChessColor::White ? 1 : -1;
    const Piece ownPawn = pawnFor(color);
    const Piece enemyPawn = enemyPawnFor(color);

    for (int f = file - 1; f <= file + 1; f += 2)
    {
        if (f < 0 || f > 7) continue;
        for (int r = rank; r >= 0 && r < 8; r -= step)
        {
            if (board.pieceAt(static_cast<Square>(r * 8 + f)) == ownPawn)
            {
                return false;
            }
        }
    }

    const int stopRank = rank + step;
    if (stopRank >= 0 && stopRank < 8)
    {
        for (int f = file - 1; f <= file + 1; f += 2)
        {
            if (f >= 0 && f < 8 && board.pieceAt(static_cast<Square>(stopRank * 8 + f)) == enemyPawn)
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

bool isLockedPawn(const Board& board, ChessColor color, Square square)
{
    const int file = fileOf(square);
    const int rank = rankOf(square);
    const int step = color == ChessColor::White ? 1 : -1;
    const int frontRank = rank + step;

    if (frontRank >= 0 && frontRank < 8)
    {
        return board.pieceAt(static_cast<Square>(frontRank * 8 + file)) == enemyPawnFor(color);
    }
    return false;
}

bool isPawnLever(const Board& board, ChessColor color, Square square)
{
    const int file = fileOf(square);
    const int rank = rankOf(square);
    const int step = color == ChessColor::White ? 1 : -1;
    const int targetRank = rank + step;
    const Piece enemyPawn = enemyPawnFor(color);

    if (targetRank < 0 || targetRank >= 8) return false;

    for (int f = file - 1; f <= file + 1; f += 2)
    {
        if (f >= 0 && f < 8 && board.pieceAt(static_cast<Square>(targetRank * 8 + f)) == enemyPawn)
        {
            return true;
        }
    }
    return false;
}

bool isIsolatedPawnOnFile(const Board& board, ChessColor color, int file)
{
    const Piece enemyPawn = enemyPawnFor(color);

    for (int r = 0; r < 8; ++r)
    {
        if (board.pieceAt(static_cast<Square>(r * 8 + file)) == enemyPawn)
        {
            bool hasNeighbor = false;
            if (file > 0 && board.pieceAt(static_cast<Square>(r * 8 + file - 1)) == enemyPawn)
                hasNeighbor = true;
            if (file < 7 && board.pieceAt(static_cast<Square>(r * 8 + file + 1)) == enemyPawn)
                hasNeighbor = true;
            if (!hasNeighbor) return true;
        }
    }
    return false;
}

int leverOpportunityScore(const Board& board, ChessColor color, Square square)
{
    const int file = fileOf(square);
    const int rank = rankOf(square);
    const int step = color == ChessColor::White ? 1 : -1;
    const int targetRank = rank + step;
    const Piece enemyPawn = enemyPawnFor(color);

    if (targetRank < 0 || targetRank >= 8) return 0;

    int score = 0;

    for (int f = file - 1; f <= file + 1; f += 2)
    {
        if (f < 0 || f > 7) continue;

        const Square leverSq = static_cast<Square>(targetRank * 8 + f);
        const Piece target = board.pieceAt(leverSq);

        if (target == enemyPawn)
        {
            const int enemyProgress = pawnProgress(color, leverSq);
            score += 6 + enemyProgress * 2;

            if (isBackwardPawn(board, color, leverSq))
            {
                score += 8;
            }

            if (isIsolatedPawnOnFile(board, color, f))
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

void computeActiveFiles(const Board& board, ChessColor color, bool activeFiles[8])
{
    const Piece pawn = pawnFor(color);
    std::fill(activeFiles, activeFiles + 8, false);

    for (int index = 0; index < 64; ++index)
    {
        const Square square = static_cast<Square>(index);
        if (board.pieceAt(square) != pawn) continue;

        const int file = fileOf(square);
        activeFiles[file] = true;
        if (file > 0) activeFiles[file - 1] = true;
        if (file < 7) activeFiles[file + 1] = true;
    }
}

}

int PawnEvaluation::evaluate(const Board& board, ChessColor color)
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

    const Piece pawn = pawnFor(color);
    int score = 0;
    std::array<int, 8> pawnsOnFile{};
    std::vector<int> pawnFileCounts;
    int passedCount = 0;
    int connectedPassedCount = 0;
    int protectedPassedCount = 0;

    bool activeFiles[8] = {};
    computeActiveFiles(board, color, activeFiles);

    for (int index = 0; index < 64; ++index)
    {
        const Square square = static_cast<Square>(index);
        if (board.pieceAt(square) != pawn)
        {
            continue;
        }

        const int file = index % 8;
        if (!activeFiles[file]) continue;

        const int rank = index / 8;
        const int progress = pawnProgress(color, square);

        pawnsOnFile[file]++;
        pawnFileCounts.push_back(file);

        const bool protectedPawn = isPawnProtected(board, color, square);
        const bool passedPawn   = isPassedPawn(board, color, square);

        score += 150 + progress * 9;

        if (isCentralPawn(square)) score += CentralBonus;
        if (protectedPawn) score += ProtectedBonus;
        if (isPhalanxPawn(board, pawn, file, rank)) score += PhalanxBonus;
        if (hasNeighborPawn(board, pawn, file, rank)) score += ConnectedBonus;

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

        if (isCandidatePassedPawn(board, color, square))
        {
            score += CandidatePassedBonusBase + std::max(0, progress - 2) * 6;
        }

        if (isPawnLever(board, color, square)) score += LeverBonus;
        if (isLockedPawn(board, color, square)) score += LockedBonus;
        if (hasSameFileNeighbor(board, pawn, file, rank)) score += PawnChainBonus;

        const int leverOpp = leverOpportunityScore(board, color, square);
        if (leverOpp > 0)
        {
            score += leverOpp;
        }

        const int spaceControl = countSpaceControl(color, square);
        if (spaceControl > 0)
        {
            score += SpaceControlBonus + std::min(8, spaceControl);
        }

        if (isBackwardPawn(board, color, square)) score -= BackwardPenalty;

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