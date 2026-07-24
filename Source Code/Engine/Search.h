#pragma once

#include "Foundation/Board.h"

class Search
{
public:
    static constexpr int MateScore = 30000;

    // depth jest liczbą półruchów analizowanych od bieżącej pozycji.
    static Move findBestMove(Board& board, int depth = 4);
    static void setPonder(bool enabled);
    static bool isPondering();

private:
    static int negamax(Board& board, int depth, int alpha, int beta, int ply);
    static int terminalScore(const Board& board, int ply);
    static int quiesce(Board& board, int alpha, int beta, int ply);
    static bool shouldStopSearch();

    static bool ponderEnabled;
};
