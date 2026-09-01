#pragma once

#include "Foundation/Bitboards.h"

class PawnStructureEvaluation
{
public:
    static int evaluate(const Board& board, const Bitboards& bitboards, ChessColor color);

    static int PieceBlocker(const Board& board, const Bitboards& bitboards, ChessColor color);
    static int PawnFear(const Board& board, const Bitboards& bitboards, ChessColor color);
    static int PieceSpace(const Board& board, const Bitboards& bitboards, ChessColor color);

    static Bitboard getAffectedPawns(const Board& board, const Move& move);
};
