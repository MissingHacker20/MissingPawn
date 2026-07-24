#include "Rate/Knight.h"

#include <cstdlib>

int KnightEvaluation::evaluate(const Board& board, ChessColor color)
{
    constexpr int Material = 320;
    const Piece knight = color == ChessColor::White ? Piece::WhiteKnight : Piece::BlackKnight;
    int score = 0;

    for (int index = 0; index < 64; ++index)
    {
        if (board.pieceAt(static_cast<Square>(index)) != knight)
        {
            continue;
        }

        const int file = index % 8;
        const int rank = index / 8;
        const int centerDistance = std::abs(file * 2 - 7) + std::abs(rank * 2 - 7);

        score += Material;
        score += 28 - centerDistance * 4;
    }

    return score;
}
