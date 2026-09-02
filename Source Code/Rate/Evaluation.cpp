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
int gamePhase(const Bitboards& b)
{
    int phase = 24;
    for (int c = 0; c < 2; ++c)
    {
        phase -= countBits(b.knights[c]) + countBits(b.bishops[c]);
        phase -= 2 * countBits(b.rooks[c]);
        phase -= 4 * countBits(b.queens[c]);
    }
    return std::max(0, std::min(24, phase));
}

int pstValue(Piece piece, int square, bool endgame)
{
    static constexpr int pawnMG[8] = {0, 50, 80, 120, 180, 280, 450, 0};
    static constexpr int pawnEG[8] = {0, 80, 120, 180, 280, 420, 650, 0};
    static constexpr int knightMG[8] = {-250, -100, 50, 120, 120, 50, -100, -250};
    static constexpr int knightEG[8] = {-150, -50, 50, 100, 100, 50, -50, -150};
    static constexpr int bishopMG[8] = {-120, -40, 40, 100, 100, 40, -40, -120};
    static constexpr int bishopEG[8] = {-80, -20, 40, 80, 80, 40, -20, -80};
    static constexpr int rookMG[8] = {0, 30, 50, 70, 70, 50, 30, 0};
    static constexpr int rookEG[8] = {80, 100, 120, 140, 140, 120, 100, 80};
    static constexpr int queenMG[8] = {-80, -20, 30, 60, 60, 30, -20, -80};
    static constexpr int queenEG[8] = {-40, 0, 40, 70, 70, 40, 0, -40};
    static constexpr int kingMG[8] = {200, 120, 40, -40, -40, 40, 120, 200};
    static constexpr int kingEG[8] = {-250, -120, 0, 80, 80, 0, -120, -250};
    const int file = square % 8;
    const int rank = square / 8;
    const int edge = std::min({file, 7 - file, rank, 7 - rank});
    int center = 3 - (std::abs(file - 3) + std::abs(rank - 3)) / 2;
    switch (piece)
    {
    case Piece::WhitePawn: case Piece::BlackPawn: return (endgame ? pawnEG[rank] : pawnMG[rank]) + center;
    case Piece::WhiteKnight: case Piece::BlackKnight: return (endgame ? knightEG[edge] : knightMG[edge]) + center;
    case Piece::WhiteBishop: case Piece::BlackBishop: return (endgame ? bishopEG[edge] : bishopMG[edge]) + center;
    case Piece::WhiteRook: case Piece::BlackRook: return (endgame ? rookEG[edge] : rookMG[edge]) + center;
    case Piece::WhiteQueen: case Piece::BlackQueen: return (endgame ? queenEG[edge] : queenMG[edge]) + center;
    case Piece::WhiteKing: case Piece::BlackKing: return endgame ? kingEG[edge] : kingMG[edge];
    default: return 0;
    }
}

int evaluatePst(const Board& board, const Bitboards& b)
{
    const int phase = gamePhase(b);
    int score = 0;
    for (int square = 0; square < 64; ++square)
    {
        const Piece p = board.pieceAt(static_cast<Square>(square));
        if (p == Piece::None) continue;
        const bool black = static_cast<int>(p) >= static_cast<int>(Piece::BlackPawn);
        const int oriented = black ? square ^ 56 : square;
        const int mg = pstValue(p, oriented, false);
        const int eg = pstValue(p, oriented, true);
        score += (black ? -1 : 1) * (mg * phase + eg * (24 - phase)) / 24;
    }
    return score;
}
}

namespace
{
int cachedPawnEvaluation(const Board& board, const Bitboards& bitboards, ChessColor color)
{
    static std::unordered_map<uint64_t, int> cache;
    const int idx = Bitboards::indexOf(color);
    uint64_t key = bitboards.pawns[idx] ^ (bitboards.pawns[1 - idx] * 0x9E3779B97F4A7C15ULL);
    key ^= bitboards.occupied[1 - idx] * 0xBF58476D1CE4E5B9ULL;
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
