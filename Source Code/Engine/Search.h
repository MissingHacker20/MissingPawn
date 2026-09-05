#pragma once

#include "Foundation/Board.h"
#include "Move/MoveList.h"

#include <vector>

class Search
{
public:
    static constexpr int MaxPly = 128;
    static constexpr int MateScore = 300000;

    // depth jest liczbą półruchów analizowanych od bieżącej pozycji.
    static Move findBestMove(Board& board, int depth = 4);
    static void setPonder(bool enabled);
    static bool isPondering();

    // PV table - zapamiętuje główną linię wyszukiwania
    static Move pvTable[MaxPly][MaxPly];
    static int pvLength[MaxPly];

    // PV z ostatniej w pełni ukończonej iteracji IDDFS
    static Move completedPvTable[MaxPly][MaxPly];
    static int completedPvLength[MaxPly];

    // Śledzenie powtórzeń na ścieżce searchu (od korzenia w dół).
    // Sprawdzamy, czy dana pozycja (klucz Zobrist) pojawiła się już
    // wcześniej na bieżącej ścieżce lub w historii gry.
    static bool enterNode(uint64_t zobristKey);
    static void exitNode(uint64_t zobristKey);

private:
    static int negamax(Board& board, int depth, int alpha, int beta, int ply);
    static int terminalScore(
        const Board& board,
        const MoveList& moves,
        int ply);
    static int quiesce(Board& board, int alpha, int beta, int ply);
    static bool shouldStopSearch();

    static bool ponderEnabled;

    // Pozycje historii i aktualnej gałęzi. Liniowe zliczanie jest ograniczone
    // do historii partii oraz bieżącej głębokości, bez hash-map alokowanej w node.
    static std::vector<uint64_t> repetitionHistory;
    static std::vector<uint64_t> repetitionPath;
};
