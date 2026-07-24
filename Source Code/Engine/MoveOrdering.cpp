#include "MoveOrdering.h"
#include "KillerMoves.h"
#include "HistoryHeuristic.h"

#include <algorithm>

constexpr int PieceValue[]
{
    0,

    100,   // WhitePawn
    320,   // WhiteKnight
    330,   // WhiteBishop
    500,   // WhiteRook
    900,   // WhiteQueen
    20000, // WhiteKing

    100,   // BlackPawn
    320,   // BlackKnight
    330,   // BlackBishop
    500,   // BlackRook
    900,   // BlackQueen
    20000  // BlackKing
};

int MoveOrdering::scoreMove(
    Board& board,
    const Move& move,
    int depth)
{
    int score = 0;
    
    //------------------------------------------
    // Killer Moves
    //------------------------------------------

    score += KillerMoves::score(
        depth,
        move);

    //------------------------------------------
    // History Heuristic
    //------------------------------------------

    score +=
        HistoryHeuristic::get(
        board.getSideToMove(),
        move);

    //------------------------------------------
    // MVV-LVA
    //------------------------------------------

    if (move.capturedPiece != Piece::None)
    {
        score +=
            PieceValue[static_cast<int>(move.capturedPiece)] * 100
            - PieceValue[static_cast<int>(move.piece)];
    }

    //------------------------------------------
    // Promotion
    //------------------------------------------

    switch (move.flag)
    {
    case MoveFlag::PromotionQueen:
    case MoveFlag::PromotionCaptureQueen:

        score += 10000;
        break;

    case MoveFlag::PromotionRook:
    case MoveFlag::PromotionCaptureRook:

        score += 9000;
        break;

    case MoveFlag::PromotionBishop:
    case MoveFlag::PromotionCaptureBishop:

        score += 8000;
        break;

    case MoveFlag::PromotionKnight:
    case MoveFlag::PromotionCaptureKnight:

        score += 8000;
        break;

    default:
        break;
    }

    //------------------------------------------
    // Castling
    //------------------------------------------

    if (move.flag == MoveFlag::KingCastle ||
        move.flag == MoveFlag::QueenCastle)
    {
        score += 50;
    }

    return score;
}

void MoveOrdering::sortMoves(
    Board& board,
    MoveList& moves,
    int depth)
{
    std::sort(
        moves.begin(),
        moves.end(),
        [&](const Move& a, const Move& b)
        {
            return scoreMove(board, a, depth) >
                   scoreMove(board, b, depth);
        });
}