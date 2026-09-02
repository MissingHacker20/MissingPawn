#include "Rate/Tactics.h"
#include "Move/MoveValidator.h"

int TacticsEvaluation::evaluate(const Board &board)
{
    // Centralny punkt dla taktyki: istniejący SEE/hanging-piece scan pozostaje
    // jedynym kosztownym przejściem, więc nie dublujemy go w evaluatorze.
    return MoveValidator::evaluateTactics(board);
}
