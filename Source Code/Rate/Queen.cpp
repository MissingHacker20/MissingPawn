#include "Rate/Queen.h"

#include <cstdlib>

int QueenEvaluation::evaluate(const Board& board, ChessColor color)
{
    constexpr int Material = 900;
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
        const int centerDistance = std::abs(file * 2 - 7) + std::abs(rank * 2 - 7);
        score += Material;
        score += 12 - centerDistance * 2;
    }

    return score;
}
