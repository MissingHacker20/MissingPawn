#include "TimeManager.h"

#include "Move/MoveGenerator.h"
#include "Rate/Evaluation.h"

std::atomic<bool> TimeManager::stopRequested{false};
bool TimeManager::infiniteSearch = false;

int TimeManager::maxDepth = 64;
int TimeManager::moveTime = -1;

int TimeManager::remainingTime[2] = {0, 0};
int TimeManager::increment[2] = {0, 0};

int TimeManager::movesToGo = 30;

uint64_t TimeManager::nodeLimit = 0;

uint64_t TimeManager::nodesSearched = 0;

int TimeManager::mateSearch = 0;

bool TimeManager::ponder = false;

int TimeManager::timeLimit = 0;

std::chrono::steady_clock::time_point
TimeManager::startTime;

int TimeManager::timeCheckCounter = 0;

void TimeManager::reset()
{
    stopRequested.store(false, std::memory_order_relaxed);

    infiniteSearch = false;

    maxDepth = 64;

    moveTime = -1;

    timeLimit = 0;

    remainingTime[0] = 0;
    remainingTime[1] = 0;

    increment[0] = 0;
    increment[1] = 0;

    nodeLimit = 0;

    nodesSearched = 0;

    mateSearch = 0;

    movesToGo = 30;

    timeCheckCounter = 0;
}

void TimeManager::start()
{
    stopRequested.store(false, std::memory_order_relaxed);

    startTime =
        std::chrono::steady_clock::now();
}

void TimeManager::setTimeLimit(int milliseconds)
{
    timeLimit = milliseconds;
}

int TimeManager::calculateTimeLimit(ChessColor side)
{
    // Jeśli ustawiono moveTime, użyj go bezpośrednio
    if (moveTime >= 0)
    {
        return moveTime;
    }

    const int sideIndex = static_cast<int>(side);
    const int time = remainingTime[sideIndex];
    const int inc = increment[sideIndex];
    const int movesToGoEst = std::max(1, movesToGo);

    if (time <= 0)
    {
        return 0; // brak kontroli czasu
    }

    // Bezpieczny przydział czasu na ruch:
    // Rozdzielamy pozostały czas na ruchy do kontroli.
    int limit = time / movesToGoEst + inc;

    // Używamy mniejszego ułamka pozostałego czasu jako sztywny limit
    int hardLimit = time / 40;

    if (limit < hardLimit)
    {
        limit = hardLimit;
    }

    if (limit < 10)
    {
        limit = 10; // minimum 10ms
    }

    // Nie używaj więcej niż 1/4 pozostałego czasu na ruch
    if (limit > time / 4)
    {
        limit = time / 4;
    }

    return limit;
}

void TimeManager::stop()
{
    stopRequested.store(true, std::memory_order_relaxed);
}

bool TimeManager::shouldStop()
{
    if (stopRequested.load(std::memory_order_relaxed))
    {
        return true;
    }

    if (infiniteSearch)
    {
        return false;
    }

    // Limit liczby węzłów (go nodes N) - sprawdzany zawsze (tani).
    if (nodeLimit > 0 && nodesSearched >= nodeLimit)
    {
        stopRequested.store(true, std::memory_order_relaxed);
        return true;
    }

    // Sprawdzanie zegara jest kosztowne (steady_clock::now()).
    // Wykonujemy je co TimeCheckInterval wywołań, aby uniknąć narzutu
    // w każdym węźle wyszukiwania. Kontrola czasu jest na tyle mało
    // wrażliwa (milisekundy), że kilka tysięcy węzłów nie ma znaczenia.
    if (++timeCheckCounter < TimeCheckInterval)
    {
        return false;
    }
    timeCheckCounter = 0;

    auto elapsed =
        std::chrono::duration_cast<
            std::chrono::milliseconds>(
                std::chrono::steady_clock::now()
                - startTime)
                .count();

    if (moveTime >= 0)
    {
        if (elapsed >= moveTime)
        {
            stopRequested.store(true, std::memory_order_relaxed);
            return true;
        }
    }

    // Sprawdź limit czasu na ruch (dla kontroli czasu wtime/btime)
    if (timeLimit > 0 && elapsed >= timeLimit)
    {
        stopRequested.store(true, std::memory_order_relaxed);
        return true;
    }

    return false;
}

void TimeManager::incrementNodeCount()
{
    ++nodesSearched;
}

void TimeManager::resetNodeCount()
{
    nodesSearched = 0;
}

uint64_t TimeManager::getNodesSearched()
{
    return nodesSearched;
}

void TimeManager::resetTimeCheckCounter()
{
    timeCheckCounter = 0;
}

void TimeManager::setDepth(
    int depth)
{
    maxDepth = depth;
}

void TimeManager::setMoveTime(
    int milliseconds)
{
    moveTime = milliseconds;
}

void TimeManager::setInfinite(
    bool value)
{
    infiniteSearch = value;
}

void TimeManager::setRemainingTime(
    ChessColor color,
    int milliseconds)
{
    remainingTime[
        static_cast<int>(color)] =
        milliseconds;
}

void TimeManager::setIncrement(
    ChessColor color,
    int milliseconds)
{
    increment[
        static_cast<int>(color)] =
        milliseconds;
}

int TimeManager::getMaxDepth()
{
    return maxDepth;
}

void TimeManager::setWhiteTime(int ms)
{
    remainingTime[static_cast<int>(ChessColor::White)] = ms;
}

void TimeManager::setBlackTime(int ms)
{
    remainingTime[static_cast<int>(ChessColor::Black)] = ms;
}

void TimeManager::setWhiteIncrement(int ms)
{
    increment[static_cast<int>(ChessColor::White)] = ms;
}

void TimeManager::setBlackIncrement(int ms)
{
    increment[static_cast<int>(ChessColor::Black)] = ms;
}

void TimeManager::setMovesToGo(int moves)
{
    movesToGo = std::max(1, moves);
}

void TimeManager::setNodeLimit(uint64_t nodes)
{
    nodeLimit = nodes;

    nodesSearched = 0;
}

void TimeManager::setMateSearch(int depth)
{
    mateSearch = depth;
}

void TimeManager::setPonder(bool enabled)
{
    ponder = enabled;
}

bool TimeManager::isPonder()
{
    return ponder;
}

uint64_t TimeManager::getNodeLimit()
{
    return nodeLimit;
}

int TimeManager::getMateSearch()
{
    return mateSearch;
}

bool TimeManager::hasTimeControl()
{
    // Sprawdzamy czy ustawiono rzeczywisty czas (a nie tylko movetime)
    return remainingTime[0] > 0 || remainingTime[1] > 0 || moveTime >= 0;
}

int TimeManager::calculateDepthFromTime(
    Board& board,
    ChessColor side)
{
    //--------------------------------------------------
    // Podstawowa głębokość z czasu
    //--------------------------------------------------

    int baseDepth;

    if (moveTime >= 0)
    {
        // Dla krótkiego czasu: mała głębokość, dla długiego: większa
if (moveTime <= 100)   baseDepth = 3;
            else if (moveTime <= 500)   baseDepth = 4;
            else if (moveTime <= 1000)  baseDepth = 5;
            else if (moveTime <= 3000)  baseDepth = 7;
            else if (moveTime <= 10000) baseDepth = 8;
            else baseDepth = 10;
    }
    else
    {
        const int sideIndex = static_cast<int>(side);
        const int time = remainingTime[sideIndex];
        const int inc = increment[sideIndex];
        const int movesToGoEst = std::max(1, movesToGo);

        if (time <= 0)
        {
            baseDepth = 6; // domyślna głębokość
        }
else
        {
            // Czas na ruch: (pozostały czas) / liczba ruchów do kontroli
            // Dzielimy przez bezpieczny współczynnik, ale nie zbyt agresywnie
            const int timePerMove = (time + inc * movesToGoEst) / movesToGoEst;

            if (timePerMove <= 100)  baseDepth = 3;
            else if (timePerMove <= 250)  baseDepth = 4;
            else if (timePerMove <= 500)  baseDepth = 5;
            else if (timePerMove <= 1000) baseDepth = 6;
            else if (timePerMove <= 2000) baseDepth = 7;
            else if (timePerMove <= 5000) baseDepth = 8;
            else if (timePerMove <= 10000) baseDepth = 9;
            else baseDepth = 10;
        }
    }

    //--------------------------------------------------
    // Uwzględnij liczbę legalnych ruchów
    //--------------------------------------------------

    // Im więcej możliwych ruchów, tym szersze drzewo -> płytsza głębokość
    MoveList legalMoves;
    const MoveValidator::CheckInfo pseudoInfo{};
    MoveGenerator::generateMoves(board, legalMoves, pseudoInfo);
    MoveValidator::filterLegalMoves(board, legalMoves);

    int moveCount = static_cast<int>(legalMoves.size());
    if (moveCount <= 5)
    {
        // Mało możliwości -> można szukać głębiej
        baseDepth += 2;
    }
    else if (moveCount <= 15)
    {
        // Normalna liczba ruchów
        // bez zmian
    }
    else if (moveCount <= 30)
    {
        // Dużo ruchów -> nieco płycej
        baseDepth -= 1;
    }
    else
    {
        // Bardzo dużo ruchów → płycej
        baseDepth -= 1;
    }

    //--------------------------------------------------
    // Uwzględnij ocenę pozycji
    //--------------------------------------------------

    // Jeśli pozycja jest jednoznacznie dobra/zła, możemy szukać głębiej
    // (silnik lepiej rozumie stabilne pozycje)
    const int eval = Evaluation::evaluate(board);

    // Uwzględnij stronę: dodajemy bezwzględną wartość ewaluacji
    const int absEval = std::abs(eval);

    if (absEval > 5000)
    {
        // Duża przewaga materialna -> pozycja prostsza, szukaj głębiej
        baseDepth += 1;
    }
    else if (absEval > 2000)
    {
        // Umiarkowana przewaga -> bez zmian
        // bez zmian
    }
    else if (absEval < 500)
    {
        // Równowaga -> pozycja subtelna, nieco płycej (bezpieczniej)
        baseDepth -= 1;
    }

    //--------------------------------------------------
    // Ogranicz do bezpiecznego zakresu
    //--------------------------------------------------

    if (baseDepth < 2)
    {
        baseDepth = 2;
    }

    if (baseDepth > 32)
    {
        baseDepth = 32;
    }

    return baseDepth;
}
