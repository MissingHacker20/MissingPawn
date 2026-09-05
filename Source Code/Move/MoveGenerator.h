#pragma once

#include "Foundation/Board.h"
#include "Foundation/Bitboards.h"
#include "MoveList.h"
#include "MoveValidator.h"

class MoveGenerator
{
public:

    // Zwraca wyłącznie legalne bicia, w tym bezpieczne en passant.
    static void generateCaptures(
        const Board& board,
        MoveList& moves,
        const MoveValidator::CheckInfo& checkInfo);

    // Jawna nazwa ścieżki używanej przez QSearch. Zachowuje tę samą
    // kolejność i semantykę co generateCaptures().
    static void generateLegalCaptures(
        const Board& board,
        MoveList& moves,
        const MoveValidator::CheckInfo& checkInfo);

    // Zwraca wyłącznie legalne ruchy. Legalność jest sprawdzana bitboardowo
    // przez MoveValidator, bez modyfikowania stanu Board.
    static void generateMoves(
        const Board& board,
        MoveList& moveList,
        const MoveValidator::CheckInfo& checkInfo);

    // Warianty korzystające z gotowego zestawu Bitboards - bitboardy biorą
    // już wygenerowane dane (figury z Board, ataki z AttackTables), nie są
    // niczym generowane specjalnie dla nich.
    static void generateCaptures(
        const Board& board,
        const Bitboards& bitboards,
        MoveList& moves,
        const MoveValidator::CheckInfo& checkInfo);

    static void generateMoves(
        const Board& board,
        const Bitboards& bitboards,
        MoveList& moveList,
        const MoveValidator::CheckInfo& checkInfo);

private:

    static void generatePawnMoves(
        const Board& board,
        const Bitboards& bitboards,
        MoveList& moveList,
        const MoveValidator::CheckInfo& checkInfo);

    static void generateKnightMoves(
        const Board& board,
        const Bitboards& bitboards,
        MoveList& moveList,
        const MoveValidator::CheckInfo& checkInfo);

    static void generateKingMoves(
        const Board& board,
        const Bitboards& bitboards,
        MoveList& moveList,
        const MoveValidator::CheckInfo& checkInfo);

    static void generateBishopMoves(
        const Board& board,
        const Bitboards& bitboards,
        MoveList& moveList,
        const MoveValidator::CheckInfo& checkInfo);

    static void generateRookMoves(
        const Board& board,
        const Bitboards& bitboards,
        MoveList& moveList,
        const MoveValidator::CheckInfo& checkInfo);

    static void generateQueenMoves(
        const Board& board,
        const Bitboards& bitboards,
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
        const Bitboards& bitboards,
        MoveList& moveList,
        const MoveValidator::CheckInfo& checkInfo);

    static void generateKnightCaptures(
        const Board& board,
        const Bitboards& bitboards,
        MoveList& moveList,
        const MoveValidator::CheckInfo& checkInfo);

    static void generateBishopCaptures(
        const Board& board,
        const Bitboards& bitboards,
        MoveList& moveList,
        const MoveValidator::CheckInfo& checkInfo);

    static void generateRookCaptures(
        const Board& board,
        const Bitboards& bitboards,
        MoveList& moveList,
        const MoveValidator::CheckInfo& checkInfo);

    static void generateQueenCaptures(
        const Board& board,
        const Bitboards& bitboards,
        MoveList& moveList,
        const MoveValidator::CheckInfo& checkInfo);

    static void generateKingCaptures(
        const Board& board,
        const Bitboards& bitboards,
        MoveList& moveList,
        const MoveValidator::CheckInfo& checkInfo);
};
