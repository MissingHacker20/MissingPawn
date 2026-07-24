#include "Rate/King.h"

int KingEvaluation::evaluate(const Board& board, ChessColor color)
{
    const Square kingSquare = board.getKingSquare(color);
    if (kingSquare == Square::None)
    {
        return 0;
    }

    const Piece pawn = color == ChessColor::White ? Piece::WhitePawn : Piece::BlackPawn;
    const int index = static_cast<int>(kingSquare);
    const int file = index % 8;
    const int rank = index / 8;
    const int shieldRank = rank + (color == ChessColor::White ? 1 : -1);
    int score = 0;

    if (shieldRank >= 0 && shieldRank < 8)
    {
        for (int shieldFile = file - 1; shieldFile <= file + 1; ++shieldFile)
        {
            if (shieldFile >= 0 && shieldFile < 8 &&
                board.pieceAt(static_cast<Square>(shieldRank * 8 + shieldFile)) == pawn)
            {
                score += 12;
            }
        }
    }

    return score;
}
