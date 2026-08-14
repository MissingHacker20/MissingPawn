#pragma once

#include "Foundation/Board.h"
#include "MoveList.h"

class MoveGenerator
{
public:

static void generateCaptures(
           Board& board,
        MoveList& moves);

    static void generateMoves(
           Board& board,
        MoveList& moveList);

private:

    static void generateKnightMoves(
        const Board& board,
        MoveList& moveList);

    static void generateKingMoves(
        const Board& board,
        MoveList& moveList);

    static void generatePawnMoves(
        const Board& board,
        MoveList& moveList);

    static void generateBishopMoves(
        const Board& board,
        MoveList& moveList);

    static void generateRookMoves(
        const Board& board,
        MoveList& moveList);

    static void generateQueenMoves(
        const Board& board,
        MoveList& moveList);

};
