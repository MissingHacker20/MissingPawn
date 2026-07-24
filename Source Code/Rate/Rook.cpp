#include "Rate/Rook.h"

namespace
{
bool hasPawnOnFile(const Board& board, Piece pawn, int file)
{
    for (int rank = 0; rank < 8; ++rank)
    {
        if (board.pieceAt(static_cast<Square>(rank * 8 + file)) == pawn)
        {
            return true;
        }
    }
    return false;
}
}

int RookEvaluation::evaluate(const Board& board, ChessColor color)
{
    constexpr int Material = 500;
    const Piece rook = color == ChessColor::White ? Piece::WhiteRook : Piece::BlackRook;
    const Piece ownPawn = color == ChessColor::White ? Piece::WhitePawn : Piece::BlackPawn;
    const Piece enemyPawn = color == ChessColor::White ? Piece::BlackPawn : Piece::WhitePawn;
    const int seventhRank = color == ChessColor::White ? 6 : 1;
    int score = 0;

    for (int index = 0; index < 64; ++index)
    {
        if (board.pieceAt(static_cast<Square>(index)) != rook)
        {
            continue;
        }

        const int file = index % 8;
        const int rank = index / 8;
        score += Material;

        if (!hasPawnOnFile(board, ownPawn, file))
        {
            score += hasPawnOnFile(board, enemyPawn, file) ? 10 : 20;
        }
        if (rank == seventhRank)
        {
            score += 20;
        }
    }

    return score;
}
