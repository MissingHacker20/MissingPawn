#pragma once

#include "Foundation/Board.h"

class PawnStructureEvaluation
{
public:
    static int evaluate(const Board& board, ChessColor color);

    static int PieceBlocker(const Board& board, ChessColor color);
    static int PawnFear(const Board& board, ChessColor color);
    static int PieceSpace(const Board& board, ChessColor color);

    static Bitboard getAffectedPawns(const Board& board, const Move& move);
};
