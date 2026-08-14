#include "Rate/Bishop.h"

#include <cstdlib>
#include <algorithm>

#include "Move/AttackTables.h"
#include "Foundation/Bitboard.h"
#include "Foundation/Board.h"

namespace
{
int countBishopMobility(const Board& board, ChessColor color, int startFile, int startRank)
{
    constexpr int Directions[4][2] = {{1,1}, {1,-1}, {-1,-1}, {-1,1}};
    int count = 0;

    for (int d = 0; d < 4; ++d)
    {
        int f = startFile + Directions[d][0];
        int r = startRank + Directions[d][1];

        while (f >= 0 && f < 8 && r >= 0 && r < 8)
        {
            Square sq = static_cast<Square>(r * 8 + f);
            Piece p = board.pieceAt(sq);
            if (p == Piece::None)
            {
                ++count;
            }
            else
            {
                if (getPieceColor(p) != color)
                    ++count; // capture square
                break;
            }
            f += Directions[d][0];
            r += Directions[d][1];
        }
    }
    return count;
}

// Sprawdza czy goniec jest "fianchetto" - na b7/g7/b2/g2 z własnym pionem przed
bool isFianchetto(const Board& board, ChessColor color, int file, int rank)
{
    const Piece ownPawn = (color == ChessColor::White) ? Piece::WhitePawn : Piece::BlackPawn;
    if (!(file == 1 || file == 6)) return false; // b lub g

    const int expectedRank = (color == ChessColor::White) ? 1 : 6;
    if (rank != expectedRank) return false;

    // Sprawdź czy pion przed gońcem
    const int pawnRank = rank + ((color == ChessColor::White) ? 1 : -1);
    if (pawnRank >= 0 && pawnRank < 8)
    {
        return board.pieceAt(static_cast<Square>(pawnRank * 8 + file)) == ownPawn;
    }
    return false;
}

// Sprawdza czy goniec jest "wielkim" (długi diagonale)
bool isLongDiagonalBishop(int file, int rank)
{
    return (file == rank) || (file + rank == 7);
}
}

int BishopEvaluation::evaluate(const Board& board, ChessColor color)
{
    constexpr int Material = 330;
    constexpr int BishopPairBonus = 40;
    constexpr int FianchettoBonus = 20;
    constexpr int LongDiagBonus = 10;
    constexpr int MobBonusPerSquare = 3;
    constexpr int MobMax = 50;

    const Piece bishop = color == ChessColor::White ? Piece::WhiteBishop : Piece::BlackBishop;
    int score = 0;
    int count = 0;

    for (int index = 0; index < 64; ++index)
    {
        if (board.pieceAt(static_cast<Square>(index)) != bishop)
        {
            continue;
        }

        ++count;
        const int file = index % 8;
        const int rank = index / 8;

        score += Material;

        // Centrum
        const double centerDist = std::abs(file - 3.5) + std::abs(rank - 3.5);
        score += 20 - static_cast<int>(centerDist * 3);

        // Fianchetto
        if (isFianchetto(board, color, file, rank))
        {
            score += FianchettoBonus;
        }

        // Długa diagonala
        if (isLongDiagonalBishop(file, rank))
        {
            score += LongDiagBonus;
        }

        // Mobilność
        const int mobility = countBishopMobility(board, color, file, rank);
        score += std::min(mobility * MobBonusPerSquare, MobMax);
    }

    // Para gońców
    if (count >= 2)
    {
        score += BishopPairBonus;
    }

    return score;
}
