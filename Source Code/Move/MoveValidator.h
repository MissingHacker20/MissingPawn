#pragma once

#include "Board.h"
#include "MoveList.h"

class MoveValidator
{
public:

    static void filterLegalMoves(
        const Board& board,
        MoveList& moveList
    );
    
    static bool isMoveLegal(
        const Board& board,
        const Move& move
    );

    static bool isKingInCheck(
        const Board& board,
        ChessColor side
    );

private:

    static ChessColor oppositeColor(ChessColor color);

    static Square findKing(
        const Board& board,
        ChessColor side
    );

    static bool isSquareAttacked(
        const Board& board,
        Square square,
        ChessColor attacker
    );
};