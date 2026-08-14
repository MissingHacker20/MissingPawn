#include "Rate/Knight.h"

#include <cstdlib>
#include <algorithm>

#include "Move/AttackTables.h"
#include "Foundation/Bitboard.h"
#include "Foundation/Board.h"

namespace
{
// Outpost: skoczek na polu wspieranym przez własnego piona,
// którego nie mogą zaatakować pionki przeciwnika
bool isKnightOutpost(const Board& board, ChessColor color, int file, int rank)
{
    const Piece ownPawn = (color == ChessColor::White) ? Piece::WhitePawn : Piece::BlackPawn;
    const int supportingRank = rank + ((color == ChessColor::White) ? -1 : 1);

    if (supportingRank >= 0 && supportingRank < 8)
    {
        bool supported = false;
        for (int f = file - 1; f <= file + 1; f += 2)
        {
            if (f >= 0 && f < 8)
            {
                if (board.pieceAt(static_cast<Square>(supportingRank * 8 + f)) == ownPawn)
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

    const Piece enemyPawn = (color == ChessColor::White) ? Piece::BlackPawn : Piece::WhitePawn;
    const int pawnAttackRank1 = rank + ((color == ChessColor::White) ? 1 : -1);
    const int pawnAttackRank2 = rank + ((color == ChessColor::White) ? 2 : -2);

    for (int f = file - 1; f <= file + 1; f += 2)
    {
        if (f >= 0 && f < 8 && pawnAttackRank1 >= 0 && pawnAttackRank1 < 8)
        {
            if (board.pieceAt(static_cast<Square>(pawnAttackRank1 * 8 + f)) == enemyPawn)
                return false;
        }
        if (f >= 0 && f < 8 && pawnAttackRank2 >= 0 && pawnAttackRank2 < 8)
        {
            if (board.pieceAt(static_cast<Square>(pawnAttackRank2 * 8 + f)) == enemyPawn)
                return false;
        }
    }

    return true;
}

int countKnightMobility(const Board& board, ChessColor color, Square from)
{
    Bitboard attacks = AttackTables::knightAttacks(from);
    attacks &= ~board.getOccupancy(color);
    return countBits(attacks);
}

bool isKnightOnEdge(int file, int rank)
{
    return (file <= 1 || file >= 6 || rank <= 1 || rank >= 6);
}
}

int KnightEvaluation::evaluate(const Board& board, ChessColor color)
{
    constexpr int Material = 320;
    constexpr int CenterBonusMax = 24;
    constexpr int EdgePenalty = 12;
    constexpr int OutpostBonus = 30;
    constexpr int MobBonusPerSquare = 4;
    constexpr int MobMax = 40;
    constexpr int ConnectedKnightsBonus = 15;

    const Piece knight = color == ChessColor::White ? Piece::WhiteKnight : Piece::BlackKnight;
    int score = 0;
    int knightCount = 0;

    for (int index = 0; index < 64; ++index)
    {
        if (board.pieceAt(static_cast<Square>(index)) != knight)
        {
            continue;
        }

        ++knightCount;
        const int file = index % 8;
        const int rank = index / 8;

        score += Material;

        const double centerDist = std::abs(file - 3.5) + std::abs(rank - 3.5);
        score += CenterBonusMax - static_cast<int>(centerDist * 5);

        if (isKnightOnEdge(file, rank))
        {
            score -= EdgePenalty;
        }

        if (isKnightOutpost(board, color, file, rank))
        {
            score += OutpostBonus;
        }

        const int mobility = countKnightMobility(board, color, static_cast<Square>(index));
        score += std::min(mobility * MobBonusPerSquare, MobMax);
    }

    if (knightCount >= 2)
    {
        score += ConnectedKnightsBonus;
    }

    return score;
}
