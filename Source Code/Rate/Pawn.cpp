#include "Rate/Pawn.h"

namespace
{
Piece pawnFor(ChessColor color)
{
    return color == ChessColor::White ? Piece::WhitePawn : Piece::BlackPawn;
}

Piece enemyPawnFor(ChessColor color)
{
    return color == ChessColor::White ? Piece::BlackPawn : Piece::WhitePawn;
}

bool isPassedPawn(const Board& board, ChessColor color, Square square)
{
    const int file = static_cast<int>(square) % 8;
    const int rank = static_cast<int>(square) / 8;
    const int step = color == ChessColor::White ? 1 : -1;

    for (int targetRank = rank + step;
         targetRank >= 0 && targetRank < 8;
         targetRank += step)
    {
        for (int targetFile = file - 1; targetFile <= file + 1; ++targetFile)
        {
            if (targetFile >= 0 && targetFile < 8 &&
                board.pieceAt(static_cast<Square>(targetRank * 8 + targetFile)) == enemyPawnFor(color))
            {
                return false;
            }
        }
    }

    return true;
}
}

int PawnEvaluation::evaluate(const Board& board, ChessColor color)
{
    constexpr int Material = 100;
    constexpr int PassedPawnBonus[8] = { 0, 0, 8, 16, 28, 45, 70, 0 };

    const Piece pawn = pawnFor(color);
    int score = 0;
    int pawnsOnFile[8] = {};

    for (int index = 0; index < 64; ++index)
    {
        const Square square = static_cast<Square>(index);
        if (board.pieceAt(square) != pawn)
        {
            continue;
        }

        const int file = index % 8;
        const int rank = index / 8;
        const int progress = color == ChessColor::White ? rank : 7 - rank;

        score += Material;
        score += progress * 4;
        pawnsOnFile[file]++;

        if (file >= 2 && file <= 5 && rank >= 3 && rank <= 4)
        {
            score += 10;
        }

        if (isPassedPawn(board, color, square))
        {
            score += PassedPawnBonus[progress];
        }
    }

    for (int file = 0; file < 8; ++file)
    {
        if (pawnsOnFile[file] > 1)
        {
            score -= (pawnsOnFile[file] - 1) * 15;
        }
    }

    return score;
}
