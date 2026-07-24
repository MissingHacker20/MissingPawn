#include "Rate/Bishop.h"

#include <cstdlib>

int BishopEvaluation::evaluate(const Board& board, ChessColor color)
{
    constexpr int Material = 330;
    constexpr int BishopPairBonus = 30;
    const Piece bishop = color == ChessColor::White ? Piece::WhiteBishop : Piece::BlackBishop;
    int score = 0;
    int count = 0;

    for (int index = 0; index < 64; ++index)
    {
        if (board.pieceAt(static_cast<Square>(index)) != bishop)
        {
            continue;
        }

        const int file = index % 8;
        const int rank = index / 8;
        const int centerDistance = std::abs(file * 2 - 7) + std::abs(rank * 2 - 7);

        score += Material;
        score += 20 - centerDistance * 3;
        ++count;
    }

    return score + (count >= 2 ? BishopPairBonus : 0);
}
