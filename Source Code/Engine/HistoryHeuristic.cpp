#include "HistoryHeuristic.h"

#include <algorithm>
#include <cstring>

int HistoryHeuristic::history[2][static_cast<int>(Piece::Count)][64][64];

bool HistoryHeuristic::isValid(const Move& move)
{
    return move.piece != Piece::None &&
           move.from != Square::None &&
           move.to != Square::None;
}

int& HistoryHeuristic::value(ChessColor side, const Move& move)
{
    return history[static_cast<int>(side)][static_cast<int>(move.piece)]
                  [static_cast<int>(move.from)][static_cast<int>(move.to)];
}

void HistoryHeuristic::clear()
{
    std::memset(history, 0, sizeof(history));
}

void HistoryHeuristic::add(
    ChessColor side,
    const Move& move,
    int depth)
{
    if (!isValid(move)) return;

    int& historyValue = value(side, move);
    const int bonus = std::min(1024, depth * depth * 8);
    historyValue += bonus - historyValue * bonus / 16384;
    historyValue = std::clamp(historyValue, -32768, 32767);
}

void HistoryHeuristic::penalize(ChessColor side, const Move& move, int depth)
{
    if (!isValid(move)) return;

    int& historyValue = value(side, move);
    const int penalty = std::min(1024, depth * depth * 4);
    historyValue -= penalty + historyValue * penalty / 16384;
    historyValue = std::clamp(historyValue, -32768, 32767);
}

int HistoryHeuristic::get(
    ChessColor side,
    const Move& move)
{
    return isValid(move) ? value(side, move) : 0;
}