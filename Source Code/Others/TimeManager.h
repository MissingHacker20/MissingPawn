#pragma once

#include <chrono>

#include "Foundation/Color.h"

class TimeManager
{
public:

    // Reset wszystkich ustawień
    static void reset();

    // Rozpoczęcie odliczania czasu
    static void start();

    // Zatrzymanie wyszukiwania przez UCI
    static void stop();

    // Czy należy zakończyć wyszukiwanie?
    static bool shouldStop();

    // Ustawienia komendy "go"
    static void setDepth(int depth);
    static void setMoveTime(int milliseconds);
    static void setInfinite(bool value);

    static void setRemainingTime(
        ChessColor color,
        int milliseconds);

    static void setIncrement(
        ChessColor color,
        int milliseconds);

    static int getMaxDepth();

    static void setWhiteTime(int ms);

    static void setBlackTime(int ms);

    static void setWhiteIncrement(int ms);

    static void setBlackIncrement(int ms);

    static void setMovesToGo(int moves);

    static void setNodeLimit(uint64_t nodes);

    static void setMateSearch(int mateDepth);

    static void setPonder(bool enabled);

    static bool isPonder();

    static uint64_t getNodeLimit();

    static int getMateSearch();

private:

    static bool stopRequested;
    static bool infiniteSearch;

    static int maxDepth;
    static int moveTime;

    static int remainingTime[2];
    static int increment[2];

    static std::chrono::steady_clock::time_point startTime;

    static int whiteTime;

    static int blackTime;

    static int whiteIncrement;

    static int blackIncrement;

    static int movesToGo;

    static uint64_t nodeLimit;

    static int mateSearch;

    static bool ponder;
};