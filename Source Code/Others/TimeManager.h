#pragma once

#include <atomic>
#include <chrono>

#include "Foundation/Color.h"

class Board;

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

// Zwiększa licznik odwiedzonych węzłów (dla limitu węzłów)
    static void incrementNodeCount();

    // Resetuje licznik odwiedzonych węzłów
    static void resetNodeCount();

    // Zwraca liczbę odwiedzonych węzłów (dla raportów UCI)
    static uint64_t getNodesSearched();

    // Resetuje licznik rzadkiego sprawdzania zegara (dla nowego wyszukiwania)
    static void resetTimeCheckCounter();

// Automatycznie oblicza głębokość na podstawie czasu
    static int calculateDepthFromTime(
        Board& board,
        ChessColor side);

// Sprawdza czy mamy ustawiony czas rzeczywisty (nie moveTime)
    static bool hasTimeControl();

    // Ustawia limit czasu na bieżący ruch (w ms)
    static void setTimeLimit(int milliseconds);

    // Oblicza limit czasu na ruch na podstawie kontroli czasu
    static int calculateTimeLimit(ChessColor side);

private:

    static std::atomic<bool> stopRequested;
    static bool infiniteSearch;

    static int maxDepth;
    static int moveTime;

    static int remainingTime[2];
    static int increment[2];

static std::chrono::steady_clock::time_point startTime;

    static int movesToGo;

static uint64_t nodeLimit;

    static uint64_t nodesSearched;

    static int mateSearch;

    static bool ponder;

// Limit czasu na ten ruch (w ms) na podstawie kontroli czasu
    static int timeLimit;

    // Czas sprawdzamy w każdym węźle. Dzięki temu movetime i kontrola
    // czasu nie są przekraczane przez długi fragment drzewa.
    static constexpr int TimeCheckInterval = 1;
    static int timeCheckCounter;
};
