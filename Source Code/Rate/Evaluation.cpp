#include "Rate/Evaluation.h"

#include "Foundation/Bitboards.h"
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
int evaluateColor(const Board& board, const Bitboards& bitboards, ChessColor color)
{
    return PawnEvaluation::evaluate(board, bitboards, color)
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
    const int tactical = MoveValidator::evaluateTactics(board);

    return evaluateColor(board, bitboards, ChessColor::White)
         - evaluateColor(board, bitboards, ChessColor::Black)
         + tactical;
}

int Evaluation::evaluate(const Board& board, const Bitboards& bitboards)
{
    const int tactical = MoveValidator::evaluateTactics(board);

    return evaluateColor(board, bitboards, ChessColor::White)
         - evaluateColor(board, bitboards, ChessColor::Black)
         + tactical;
}
