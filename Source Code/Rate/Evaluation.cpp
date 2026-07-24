#include "Rate/Evaluation.h"

#include "Rate/Bishop.h"
#include "Rate/King.h"
#include "Rate/Knight.h"
#include "Rate/Pawn.h"
#include "Rate/Queen.h"
#include "Rate/Rook.h"

namespace
{
int evaluateColor(const Board& board, ChessColor color)
{
    return PawnEvaluation::evaluate(board, color)
        + KnightEvaluation::evaluate(board, color)
        + BishopEvaluation::evaluate(board, color)
        + RookEvaluation::evaluate(board, color)
        + QueenEvaluation::evaluate(board, color)
        + KingEvaluation::evaluate(board, color);
}
}

int Evaluation::evaluate(const Board& board)
{
    return evaluateColor(board, ChessColor::White)
        - evaluateColor(board, ChessColor::Black);
}
