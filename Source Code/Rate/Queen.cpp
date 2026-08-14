#include "Rate/Queen.h"

#include <cstdlib>
#include <algorithm>

#include "Move/AttackTables.h"
#include "Foundation/Bitboard.h"
#include "Foundation/Board.h"

namespace
{
int countQueenMobility(const Board& board, ChessColor color, int startFile, int startRank)
{
    constexpr int Directions[8][2] = {
        {0,1}, {1,0}, {0,-1}, {-1,0}, // rook
        {1,1}, {1,-1}, {-1,-1}, {-1,1} // bishop
    };
    int count = 0;

    for (int d = 0; d < 8; ++d)
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
                    ++count;
                break;
            }
            f += Directions[d][0];
            r += Directions[d][1];
        }
    }
    return count;
}

// Sprawdza czy hetman jest za wcześnie wyprowadzony (early queen development penalty)
bool isEarlyQueenDevelopment(const Board& board, ChessColor color, int rank)
{
    int devRank = (color == ChessColor::White) ? rank : 7 - rank;

    // Jeśli hetman opuścił swój początkowy rząd przed rozwinięciem innych figur
    if (devRank >= 2)
    {
        // Sprawdź czy skoczkowie i gońce są jeszcze na miejscu
        Piece knight = (color == ChessColor::White) ? Piece::WhiteKnight : Piece::BlackKnight;
        Piece bishop = (color == ChessColor::White) ? Piece::WhiteBishop : Piece::BlackBishop;

        int knightsHome = countBits(board.getBitboard(knight));
        int bishopsHome = countBits(board.getBitboard(bishop));

        // Jeśli większość lekkich figur jeszcze nie rozwinięta
        return (knightsHome + bishopsHome >= 3);
    }
    return false;
}
}

int QueenEvaluation::evaluate(const Board& board, ChessColor color)
{
    constexpr int Material = 900;
    constexpr int MobBonusPerSquare = 2;
    constexpr int MobMax = 60;
    constexpr int EarlyQueenPenalty = 30;

    const Piece queen = color == ChessColor::White ? Piece::WhiteQueen : Piece::BlackQueen;
    int score = 0;

    for (int index = 0; index < 64; ++index)
    {
        if (board.pieceAt(static_cast<Square>(index)) != queen)
        {
            continue;
        }

        const int file = index % 8;
        const int rank = index / 8;

        score += Material;

        // Centrum (delikatny bonus)
        const double centerDist = std::abs(file - 3.5) + std::abs(rank - 3.5);
        score += 12 - static_cast<int>(centerDist * 2);

        // Kara za wczesne wyprowadzenie
        if (isEarlyQueenDevelopment(board, color, rank))
        {
            score -= EarlyQueenPenalty;
        }

        // Mobilność
        const int mobility = countQueenMobility(board, color, file, rank);
        score += std::min(mobility * MobBonusPerSquare, MobMax);
    }

    return score;
}
