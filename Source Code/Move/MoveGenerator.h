#pragma once

#include "Foundation/Board.h"
#include "Foundation/Move.h"
#include "MoveList.h"

class MoveGenerator
{
public:

    static void generateCaptures(
        const Board& board,
        MoveList& moves);

    static void generateMoves(
        const Board& board,
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

    static void generateSlidingMoves(
    const Board& board,
    MoveList& moveList,
    Piece piece,
    const int directions[][2],
    int directionCount);
};