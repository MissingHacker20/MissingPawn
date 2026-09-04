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
    int& value = history[(int)side][(int)move.from][(int)move.to];
    const int bonus = std::min(1024, depth * depth * 8);
    value += bonus - value * bonus / 16384;
}

void HistoryHeuristic::penalize(ChessColor side, const Move& move, int depth)
{
    int& value = history[(int)side][(int)move.from][(int)move.to];
    const int penalty = std::min(1024, depth * depth * 4);
    value -= penalty + value * penalty / 16384;
}

int HistoryHeuristic::get(
    ChessColor side,
    const Move& move)
{
    return history[(int)side]
                  [(int)move.from]
                  [(int)move.to];
}