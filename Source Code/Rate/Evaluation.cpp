#include "Rate/Evaluation.h"

#include "Rate/Bishop.h"
#include "Rate/King.h"
#include "Rate/Knight.h"
#include "Rate/Pawn.h"
#include "Rate/PawnStructure.h"
#include "Rate/Queen.h"
#include "Rate/Rook.h"
#include "Move/MoveValidator.h"

namespace
{
int evaluateColor(const Board& board, ChessColor color)
{
    return PawnEvaluation::evaluate(board, color)
        + PawnStructureEvaluation::evaluate(board, color)
        + KnightEvaluation::evaluate(board, color)
        + BishopEvaluation::evaluate(board, color)
        + RookEvaluation::evaluate(board, color)
        + QueenEvaluation::evaluate(board, color)
        + KingEvaluation::evaluate(board, color);
}
}

int Evaluation::evaluate(const Board& board)
{
    // Ocena taktyczna (SEE / wiszące figury) jest liczona JEDEN raz
    // dla całej planszy i zwraca wynik netto (biały - czarny), co
    // połowę kosztu w porównaniu do wywoływania jej osobno dla
    // każdego koloru.
    const int tactical = MoveValidator::evaluateTactics(board);

    return evaluateColor(board, ChessColor::White)
         - evaluateColor(board, ChessColor::Black)
         + tactical;
}
