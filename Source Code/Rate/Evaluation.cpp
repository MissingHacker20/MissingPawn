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
    static constexpr int pawnMG[8] = {0, 50, 80, 120, 180, 280, 450, 0};
    static constexpr int pawnEG[8] = {0, 80, 120, 180, 280, 420, 650, 0};
    // Pełne PST: pole jest parametrem, a nie funkcją odległości od krawędzi.
    static constexpr int knightMG[64] = {
        -250,-180,-120,-80,-80,-120,-180,-250, -180,-80,0,40,40,0,-80,-180,
        -120,0,80,120,120,80,0,-120, -80,40,120,180,180,120,40,-80,
        -80,40,120,180,180,120,40,-80, -120,0,80,120,120,80,0,-120,
        -180,-80,0,40,40,0,-80,-180, -250,-180,-120,-80,-80,-120,-180,-250};
    static constexpr int knightEG[64] = {
        -150,-100,-60,-40,-40,-60,-100,-150, -100,-40,0,30,30,0,-40,-100,
        -60,0,40,70,70,40,0,-60, -40,30,70,100,100,70,30,-40,
        -40,30,70,100,100,70,30,-40, -60,0,40,70,70,40,0,-60,
        -100,-40,0,30,30,0,-40,-100, -150,-100,-60,-40,-40,-60,-100,-150};
    static constexpr int bishopMG[64] = {
        -120,-80,-60,-40,-40,-60,-80,-120, -80,-20,0,20,20,0,-20,-80,
        -60,0,40,60,60,40,0,-60, -40,20,60,100,100,60,20,-40,
        -40,20,60,100,100,60,20,-40, -60,0,40,60,60,40,0,-60,
        -80,-20,0,20,20,0,-20,-80, -120,-80,-60,-40,-40,-60,-80,-120};
    static constexpr int bishopEG[64] = {
        -80,-50,-30,-20,-20,-30,-50,-80, -50,-10,10,20,20,10,-10,-50,
        -30,10,40,50,50,40,10,-30, -20,20,50,70,70,50,20,-20,
        -20,20,50,70,70,50,20,-20, -30,10,40,50,50,40,10,-30,
        -50,-10,10,20,20,10,-10,-50, -80,-50,-30,-20,-20,-30,-50,-80};
    static constexpr int rookMG[8] = {0, 30, 50, 70, 70, 50, 30, 0};
    static constexpr int rookEG[8] = {80, 100, 120, 140, 140, 120, 100, 80};
    static constexpr int queenMG[8] = {-80, -20, 30, 60, 60, 30, -20, -80};
    static constexpr int queenEG[8] = {-40, 0, 40, 70, 70, 40, 0, -40};
    static constexpr int kingMG[8] = {200, 120, 40, -40, -40, 40, 120, 200};
    static constexpr int kingEG[8] = {-250, -120, 0, 80, 80, 0, -120, -250};
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

int evaluatePst(const Board& board, const Bitboards& b)
{
    const int phase = Evaluation::gamePhase(b);
    int score = 0;

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
            score += (black ? -1 : 1) * (mg * phase + eg * (24 - phase)) / 24;
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
int cachedPawnEvaluation(const Board& board, const Bitboards& bitboards, ChessColor color)
{
    static std::unordered_map<uint64_t, int> cache;
    const int idx = Bitboards::indexOf(color);
    // Klucz obejmuje cały kontekst używany przez ocenę pionów: oba
    // bitboardy pionów, zajętość obu stron i pełną zajętość pozycji.
    // Dzięki temu cache nie zwraca wyniku dla innego układu figur.
    uint64_t key = bitboards.pawns[idx] ^
        (bitboards.pawns[1 - idx] * 0x9E3779B97F4A7C15ULL);
    key ^= bitboards.occupied[idx] * 0xD6E8FEB86659FD93ULL;
    key ^= bitboards.occupied[1 - idx] * 0xBF58476D1CE4E5B9ULL;
    key ^= bitboards.allOccupied * 0xA24BAED4963EE407ULL;
    key ^= static_cast<uint64_t>(idx) * 0x94D049BB133111EBULL;
    const auto found = cache.find(key);
    if (found != cache.end()) return found->second;
    const int value = PawnEvaluation::evaluate(board, bitboards, color);
    if (cache.size() > 32768) cache.clear();
    cache.emplace(key, value);
    return value;
}

int evaluateColor(const Board& board, const Bitboards& bitboards, ChessColor color)
{
    return cachedPawnEvaluation(board, bitboards, color)
        + PawnStructureEvaluation::evaluate(board, bitboards, color)
        + KnightEvaluation::evaluate(board, bitboards, color)
        + BishopEvaluation::evaluate(board, bitboards, color)
        + RookEvaluation::evaluate(board, bitboards, color)
        + QueenEvaluation::evaluate(board, bitboards, color)
        + KingEvaluation::evaluate(board, bitboards, color);
}
}

int Evaluation::evaluate(const Board& board)
{
    // Bitboardy liczone JEDEN raz na całą ocenę - na podstawie danych już
    // wygenerowanych przez silnik (Board, AttackTables, CheckInfo).
    const Bitboards bitboards = Bitboards::compute(board, true);

    // Ocena taktyczna (SEE / wiszące figury) jest liczona JEDEN raz
    // dla całej planszy i zwraca wynik netto (biały - czarny), co
    // połowę kosztu w porównaniu do wywoływania jej osobno dla
    // każdego koloru.
    const int tactical = TacticsEvaluation::evaluate(board);

    return evaluateColor(board, bitboards, ChessColor::White)
         - evaluateColor(board, bitboards, ChessColor::Black)
         + evaluatePst(board, bitboards) + tactical;
}

int Evaluation::evaluate(const Board& board, const Bitboards& bitboards)
{
    const int tactical = TacticsEvaluation::evaluate(board);

    return evaluateColor(board, bitboards, ChessColor::White)
         - evaluateColor(board, bitboards, ChessColor::Black)
         + evaluatePst(board, bitboards) + tactical;
}
