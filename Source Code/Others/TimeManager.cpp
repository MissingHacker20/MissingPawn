#include "TimeManager.h"

bool TimeManager::stopRequested = false;
bool TimeManager::infiniteSearch = false;

int TimeManager::maxDepth = 64;
int TimeManager::moveTime = -1;

int TimeManager::remainingTime[2] = {0, 0};
int TimeManager::increment[2] = {0, 0};

int TimeManager::whiteTime = 0;

int TimeManager::blackTime = 0;

int TimeManager::whiteIncrement = 0;

int TimeManager::blackIncrement = 0;

int TimeManager::movesToGo = 30;

uint64_t TimeManager::nodeLimit = 0;

int TimeManager::mateSearch = 0;

bool TimeManager::ponder = false;

std::chrono::steady_clock::time_point
TimeManager::startTime;

void TimeManager::reset()
{
    stopRequested = false;

    infiniteSearch = false;

    maxDepth = 64;

    moveTime = -1;

    remainingTime[0] = 0;
    remainingTime[1] = 0;

    increment[0] = 0;
    increment[1] = 0;
}

void TimeManager::start()
{
    stopRequested = false;

    startTime =
        std::chrono::steady_clock::now();
}

void TimeManager::stop()
{
    stopRequested = true;
}

bool TimeManager::shouldStop()
{
    if (stopRequested)
    {
        return true;
    }

    if (infiniteSearch)
    {
        return false;
    }

    if (moveTime >= 0)
    {
        auto elapsed =
            std::chrono::duration_cast<
                std::chrono::milliseconds>(
                    std::chrono::steady_clock::now()
                    - startTime)
                    .count();

        if (elapsed >= moveTime)
        {
            return true;
        }
    }

    return false;
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
    whiteTime = ms;
}

void TimeManager::setBlackTime(int ms)
{
    blackTime = ms;
}

void TimeManager::setWhiteIncrement(int ms)
{
    whiteIncrement = ms;
}

void TimeManager::setBlackIncrement(int ms)
{
    blackIncrement = ms;
}

void TimeManager::setMovesToGo(int moves)
{
    movesToGo = std::max(1, moves);
}

void TimeManager::setNodeLimit(uint64_t nodes)
{
    nodeLimit = nodes;
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