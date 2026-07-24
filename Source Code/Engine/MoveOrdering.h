#pragma once

#include "Foundation/Board.h"
#include "Foundation/Move.h"
#include "Move/MoveList.h"

class MoveOrdering
{
public:

    static void sortMoves(
        Board& board,
        MoveList& moves, int depth);

private:

    static int scoreMove(
    Board& board,
    const Move& move,
    int depth);
};