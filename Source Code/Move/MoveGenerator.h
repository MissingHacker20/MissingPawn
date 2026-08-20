#pragma once

#include "Foundation/Board.h"
#include "MoveList.h"
#include "MoveValidator.h"

class MoveGenerator
{
public:

    static void generateCaptures(
        const Board& board,
        MoveList& moves,
        const MoveValidator::CheckInfo& checkInfo);

    static void generateMoves(
        const Board& board,
        MoveList& moveList,
        const MoveValidator::CheckInfo& checkInfo);

private:

    static void generatePawnMoves(
        const Board& board,
        MoveList& moveList,
        const MoveValidator::CheckInfo& checkInfo);

    static void generateKnightMoves(
        const Board& board,
        MoveList& moveList,
        const MoveValidator::CheckInfo& checkInfo);

    static void generateKingMoves(
        const Board& board,
        MoveList& moveList,
        const MoveValidator::CheckInfo& checkInfo);

    static void generateBishopMoves(
        const Board& board,
        MoveList& moveList,
        const MoveValidator::CheckInfo& checkInfo);

    static void generateRookMoves(
        const Board& board,
        MoveList& moveList,
        const MoveValidator::CheckInfo& checkInfo);

    static void generateQueenMoves(
        const Board& board,
        MoveList& moveList,
        const MoveValidator::CheckInfo& checkInfo);

    // Helper: generate castling moves
    static void generateCastlingMoves(
        const Board& board,
        MoveList& moveList,
        const MoveValidator::CheckInfo& checkInfo);

    // Helper: add moves for a sliding piece restricted by pin/evation mask
    static void generateSliderMoves(
        const Board& board,
        MoveList& moveList,
        const MoveValidator::CheckInfo& checkInfo,
        Piece piece,
        Bitboard pieceBitboard,
        Bitboard occupancy,
        Bitboard allowedSquares);
        
    // Helper functions for capture generation
    static void generatePawnCaptures(
        const Board& board,
        MoveList& moveList,
        const MoveValidator::CheckInfo& checkInfo);
        
    static void generateKnightCaptures(
        const Board& board,
        MoveList& moveList,
        const MoveValidator::CheckInfo& checkInfo);
        
    static void generateBishopCaptures(
        const Board& board,
        MoveList& moveList,
        const MoveValidator::CheckInfo& checkInfo);
        
    static void generateRookCaptures(
        const Board& board,
        MoveList& moveList,
        const MoveValidator::CheckInfo& checkInfo);
        
    static void generateQueenCaptures(
        const Board& board,
        MoveList& moveList,
        const MoveValidator::CheckInfo& checkInfo);
        
    static void generateKingCaptures(
        const Board& board,
        MoveList& moveList,
        const MoveValidator::CheckInfo& checkInfo);
};
