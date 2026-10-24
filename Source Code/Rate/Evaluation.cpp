/*
 * MissingPawn
 * Copyright (C) 2026 Kacper Wieczorek
 *
 * This file is part of MissingPawn.
 *
 * Licensed under the GNU General Public License v3 or later.
 * See LICENSE for details.
 */

#include "Rate/Evaluation.h"

#include "Foundation/Bitboards.h"
#include "Rate/Bishop.h"
#include "Rate/King.h"
#include "Rate/Knight.h"
#include "Rate/Pawn.h"
#include "Rate/PawnStructure.h"
#include "Rate/Queen.h"
#include "Rate/Rook.h"
#include "Rate/Tactics.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace
{
int pstValue(Piece piece, int square, bool endgame)
{
    static constexpr int pawnMG[8] = {27,60,-44,-24,54,107,106,27};
    static constexpr int pawnEG[8] = {206,280,130,103,206,291,294,206};
    // Pełne PST: pole jest parametrem, a nie funkcją odległości od krawędzi.
    static constexpr int knightMG[64] = {-285,-7,-223,-198,-156,-213,-102,-262,-240,-109,-74,232,207,89,-120,-83,-355,-163,72,107,33,276,38,-315,-121,4,181,248,423,28,115,50,-60,206,62,398,199,207,107,-1,-165,19,112,165,206,186,94,-94,-225,-89,-4,101,120,58,-1,-180,-216,-171,-74,-31,-42,-69,-172,-248};
    static constexpr int knightEG[64] = {-181,-206,-154,-135,-159,-125,-185,-163,-151,-26,-113,-49,-11,-84,-71,-123,-137,-47,-192,-20,-107,-187,-112,-137,-32,-20,10,44,32,-49,25,-55,-12,54,4,117,11,78,-18,-13,-122,24,69,-20,79,54,63,-113,-113,-1,-48,86,60,-23,-27,-137,-107,-97,17,-22,-11,-21,-103,-171};
    static constexpr int bishopMG[64] = {-243,-130,-120,-182,-144,-214,-145,-221,-94,-167,39,25,160,-25,96,-120,-201,28,-1,177,83,28,34,-150,-53,66,187,124,198,158,14,-17,-23,207,99,221,164,162,103,-10,129,42,113,118,122,156,108,166,-58,-124,-2,43,52,74,-45,-130,-32,27,-52,20,-18,2,-41,-61};
    static constexpr int bishopEG[64] = {-219,-116,-244,-144,-119,-251,-88,-138,-91,-189,-50,-62,1,-42,-193,-140,-140,-46,-26,49,19,-118,-38,-75,-20,6,85,79,4,25,-70,-14,42,84,66,66,64,90,28,8,36,8,26,28,58,80,26,65,2,-37,27,-61,-1,45,-6,-101,-22,-13,-79,11,-12,7,-10,-48};
    static constexpr int rookMG[8] = {66,153,178,176,176,178,153,66};
    static constexpr int rookEG[8] = {259,255,250,249,249,250,255,259};
    static constexpr int queenMG[8] = {-339,-342,-276,-181,-181,-276,-342,-339};
    static constexpr int queenEG[8] = {74,70,60,73,73,60,70,74};
    static constexpr int kingMG[8] = {37,-97,13,-111,-111,13,-97,37};
    static constexpr int kingEG[8] = {-489,-126,-69,-118,-118,-69,-126,-489};
    const int file = square % 8;
    const int rank = square / 8;
    switch (piece)
    {
    case Piece::WhitePawn: case Piece::BlackPawn: return endgame ? pawnEG[rank] : pawnMG[rank];
    case Piece::WhiteKnight: case Piece::BlackKnight: return endgame ? knightEG[square] : knightMG[square];
    case Piece::WhiteBishop: case Piece::BlackBishop: return endgame ? bishopEG[square] : bishopMG[square];
    case Piece::WhiteRook: case Piece::BlackRook: return endgame ? rookEG[std::min(file, 7-file)] : rookMG[std::min(file, 7-file)];
    case Piece::WhiteQueen: case Piece::BlackQueen: return endgame ? queenEG[std::min(file, 7-file)] : queenMG[std::min(file, 7-file)];
    case Piece::WhiteKing: case Piece::BlackKing:
    {
        const int edge = std::min({file, 7 - file, rank, 7 - rank});
        return endgame ? kingEG[edge] : kingMG[edge];
    }
    default: return 0;
    }
}

struct ScorePair
{
    int mg = 0;
    int eg = 0;
};



ScorePair evaluatePst(const Board& board, const Bitboards& b)
{
    ScorePair score;

    // Odwiedzaj wyłącznie zajęte pola. Poza mniejszą liczbą odczytów mailbox
    // zachowujemy dokładnie tę samą orientację PST dla czarnych figur.
    const auto addPiecePst = [&](Bitboard pieces, Piece piece, bool black)
    {
        while (pieces)
        {
            const int square = static_cast<int>(popLeastSignificantBit(pieces));
            const int oriented = black ? (square ^ 56) : square;
            const int mg = pstValue(piece, oriented, false);
            const int eg = pstValue(piece, oriented, true);
            const int sign = black ? -1 : 1;
            score.mg += sign * mg;
            score.eg += sign * eg;
        }
    };

    addPiecePst(b.pawns[0], Piece::WhitePawn, false);
    addPiecePst(b.knights[0], Piece::WhiteKnight, false);
    addPiecePst(b.bishops[0], Piece::WhiteBishop, false);
    addPiecePst(b.rooks[0], Piece::WhiteRook, false);
    addPiecePst(b.queens[0], Piece::WhiteQueen, false);
    addPiecePst(b.kings[0], Piece::WhiteKing, false);
    addPiecePst(b.pawns[1], Piece::BlackPawn, true);
    addPiecePst(b.knights[1], Piece::BlackKnight, true);
    addPiecePst(b.bishops[1], Piece::BlackBishop, true);
    addPiecePst(b.rooks[1], Piece::BlackRook, true);
    addPiecePst(b.queens[1], Piece::BlackQueen, true);
    addPiecePst(b.kings[1], Piece::BlackKing, true);

    (void)board;
    return score;
}

int taperedScore(const ScorePair& score, int phase)
{
    return (score.mg * phase + score.eg * (24 - phase)) / 24;
}
}

int Evaluation::gamePhase(const Bitboards& bitboards)
{
    int phase = 0;
    for (int c = 0; c < 2; ++c)
    {
        phase += countBits(bitboards.knights[c]) + countBits(bitboards.bishops[c]);
        phase += 2 * countBits(bitboards.rooks[c]);
        phase += 4 * countBits(bitboards.queens[c]);
    }
    return std::max(0, std::min(24, phase));
}

namespace
{
std::pair<int, int> cachedPawnEvaluation(const Board& board, const Bitboards& bitboards, ChessColor color)
{
    return PawnEvaluation::evaluatePair(board, bitboards, color);
}

ScorePair evaluateColor(const Board& board, const Bitboards& bitboards, ChessColor color)
{
    const auto pawn = cachedPawnEvaluation(board, bitboards, color);
    const auto structure = PawnStructureEvaluation::evaluatePair(board, bitboards, color);
    const auto knight = KnightEvaluation::evaluatePair(board, bitboards, color);
    const auto bishop = BishopEvaluation::evaluatePair(board, bitboards, color);
    const auto rook = RookEvaluation::evaluatePair(board, bitboards, color);
    const auto queen = QueenEvaluation::evaluatePair(board, bitboards, color);
    const auto king = KingEvaluation::evaluatePair(board, bitboards, color);
    return {pawn.first + structure.first + knight.first + bishop.first + rook.first + queen.first + king.first,
            pawn.second + structure.second + knight.second + bishop.second + rook.second + queen.second + king.second};
}
}

int Evaluation::evaluate(const Board& board)
{
    // Bitboardy liczone JEDEN raz na całą ocenę - na podstawie danych już
    // wygenerowanych przez silnik (Board, AttackTables, CheckInfo).
    const Bitboards bitboards = Bitboards::computeHeavy(board);

    // Ocena taktyczna (SEE / wiszące figury) jest liczona JEDEN raz
    // dla całej planszy i zwraca wynik netto (biały - czarny), co
    // połowę kosztu w porównaniu do wywoływania jej osobno dla
    // każdego koloru.
    const int tactical = TacticsEvaluation::evaluate(board);

    const int phase = gamePhase(bitboards);
    const ScorePair white = evaluateColor(board, bitboards, ChessColor::White);
    const ScorePair black = evaluateColor(board, bitboards, ChessColor::Black);
    const ScorePair pst = evaluatePst(board, bitboards);
    const ScorePair total{
        white.mg - black.mg + pst.mg + tactical,
        white.eg - black.eg + pst.eg + tactical};
    return taperedScore(total, phase);
}

int Evaluation::evaluate(const Board& board, const Bitboards& bitboards)
{
    const int tactical = TacticsEvaluation::evaluate(board);
    const int phase = gamePhase(bitboards);
    const ScorePair white = evaluateColor(board, bitboards, ChessColor::White);
    const ScorePair black = evaluateColor(board, bitboards, ChessColor::Black);
    const ScorePair pst = evaluatePst(board, bitboards);
    const ScorePair total{
        white.mg - black.mg + pst.mg + tactical,
        white.eg - black.eg + pst.eg + tactical};
    return taperedScore(total, phase);
}
