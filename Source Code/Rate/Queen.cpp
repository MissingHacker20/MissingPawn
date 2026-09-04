#include "Rate/Queen.h"
#include "Rate/Evaluation.h"

#include <cstdlib>
#include <algorithm>

#include "Move/AttackTables.h"
#include "Foundation/Bitboard.h"
#include "Foundation/Board.h"

namespace
{
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

int QueenEvaluation::evaluate(const Board& /*board*/, const Bitboards& bitboards, ChessColor color)
{
    constexpr int Material = 9000;
    constexpr int MobBonusPerSquareMG = 12;
    constexpr int MobBonusPerSquareEG = 8;
    constexpr int MobMax = 400;
    constexpr int VulnerableToMinorOrPawnPenalty = 200;
    constexpr int EarlyQueenPenalty = 100;

    const int idx = Bitboards::indexOf(color);
    const int phase = Evaluation::gamePhase(bitboards);
    int score = 0;

    Bitboard queens = bitboards.queens[idx];
    while (queens)
    {
        const Square square = popLeastSignificantBit(queens);
        const int index = static_cast<int>(square);
        const int file = index % 8;
        const int rank = index / 8;

        score += Material;

        // Centrum (delikatny bonus)
        const double centerDist = std::abs(file - 3.5) + std::abs(rank - 3.5);
        score += 70 - static_cast<int>(centerDist * 12);

        // Early development is a problem only when the queen is actually
        // exposed to attack while the minor pieces still block development.
        if (queenIsExposedEarly(bitboards, color, square))
        {
            score -= EarlyQueenPenalty;
        }

        const int mobility = countQueenMobility(bitboards, color, square);
        const int mobBonus = (MobBonusPerSquareMG * phase + MobBonusPerSquareEG * (24 - phase)) / 24;
        score += std::max(0, std::min(mobility * mobBonus, MobMax) - 50);

        const ChessColor enemy = color == ChessColor::White ? ChessColor::Black : ChessColor::White;
        const Bitboard enemyMinorPawn = bitboards.pawnAttacks[Bitboards::indexOf(enemy)] |
            bitboards.knightAttacks[Bitboards::indexOf(enemy)] |
            bitboards.bishopAttacks[Bitboards::indexOf(enemy)];
        if (getBit(enemyMinorPawn, square)) score -= VulnerableToMinorOrPawnPenalty;
    }

    return score;
}
