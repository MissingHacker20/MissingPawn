#include "Foundation/Board.h"
#include "Game/Fen.h"
#include "Move/MoveGenerator.h"

#include <cstdint>
#include <iostream>

namespace
{
    uint64_t perft(Board &board, int depth)
    {
        if (depth == 0)
            return 1;

        MoveList moves;
        MoveGenerator::generateMoves(board, moves, MoveValidator::CheckInfo{});

        uint64_t nodes = 0;
        for (int i = 0; i < moves.size(); ++i)
        {
            UndoInfo undo;
            board.makeMove(moves[i], undo);
            nodes += perft(board, depth - 1);
            board.undoMove(moves[i], undo);
        }
        return nodes;
    }

    bool expectPerft(Board &board, const char *fen, int depth, uint64_t expected)
    {
        if (!Fen::load(board, fen))
        {
            std::cerr << "Invalid FEN: " << fen << '\n';
            return false;
        }

        const uint64_t actual = perft(board, depth);
        if (actual == expected)
            return true;

        std::cerr << "Perft mismatch at depth " << depth
                  << ": expected " << expected << ", got " << actual << '\n';
        return false;
    }
}

int main()
{
    Board board;
    board.setStartPosition();
    bool passed = perft(board, 2) == 400;

    passed &= expectPerft(board, "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", 2, 568);
    passed &= expectPerft(board, "r3k2r/8/8/3pP3/8/8/8/R3K2R w KQkq d6 0 1", 2, 648);

    return passed ? 0 : 1;
}
