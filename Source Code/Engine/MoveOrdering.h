#pragma once

#include "Foundation/Board.h"
#include "Foundation/Move.h"
#include "Move/MoveList.h"

class MoveOrdering
{
public:

    static void sortMoves(
        Board& board,
        MoveList& moves,
        int depth,
        int ply,
        const Move& ttMove = Move{},
        int cachedTactical = 0);

private:

    static int scoreMove(
    Board& board,
    const Move& move,
    int depth,
    int ply,
    const Move& ttMove,
    int cachedTactical = 0);

    static bool isCapture(const Move& move);

    static bool isPromotion(const Move& move);
};
