#include "HistoryHeuristic.h"

#include <cstring>

int HistoryHeuristic::history[2][64][64];

void HistoryHeuristic::clear()
{
    for (int side = 0; side < 2; side++)
    {
        for (int from = 0; from < 64; from++)
        {
            for (int to = 0; to < 64; to++)
            {
                history[side][from][to] = 0;
            }
        }
    }
}

void HistoryHeuristic::add(
    ChessColor side,
    const Move& move,
    int depth)
{
    history[(int)side]
           [(int)move.from]
           [(int)move.to]
        += depth * depth;
}

int HistoryHeuristic::get(
    ChessColor side,
    const Move& move)
{
    return history[(int)side]
                  [(int)move.from]
                  [(int)move.to];
}