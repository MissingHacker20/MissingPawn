#pragma once

#include "Foundation/Color.h"
#include "Foundation/Move.h"

class HistoryHeuristic
{
public:

    static void add(
        ChessColor side,
        const Move& move,
        int depth);

    static void penalize(
        ChessColor side,
        const Move& move,
        int depth);

    static int get(
        ChessColor side,
        const Move& move);

    static void clear();


private:

    // Historia rozróżnia stronę i typ bierki, aby np. ruch skoczkiem
    // nie wpływał na kolejność ruchów wieży z tego samego pola.
    static int history[2][static_cast<int>(Piece::Count)][64][64];

    static int& value(ChessColor side, const Move& move);
    static bool isValid(const Move& move);
};