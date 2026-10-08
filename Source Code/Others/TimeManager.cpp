/*
 * MissingPawn
 * Copyright (C) 2026 Kacper Wieczorek
 *
 * This file is part of MissingPawn.
 *
 * Licensed under the GNU General Public License v3 or later.
 * See LICENSE for details.
 */

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
        return 0;
    }

    int limit = time / movesToGoEst + inc;

    int hardLimit = time / 40;

    if (limit < hardLimit)
    {
        limit = hardLimit;
    }

    if (limit < 10)
    {
        limit = 10;
    }

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

    if (nodeLimit > 0 && nodesSearched >= nodeLimit)
    {
        stopRequested.store(true, std::memory_order_relaxed);
        return true;
    }

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
    return remainingTime[0] > 0 || remainingTime[1] > 0 || moveTime >= 0 || nodeLimit > 0;
}

int TimeManager::calculateDepthFromTime(
    Board& board,
    ChessColor side)
{
    int baseDepth;

    if (moveTime >= 0)
    {
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
            baseDepth = 6;
        }
        else
        {
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

    MoveList legalMoves;
    const MoveValidator::CheckInfo pseudoInfo{};
    MoveGenerator::generateMoves(board, legalMoves, pseudoInfo);

    int moveCount = static_cast<int>(legalMoves.size());
    if (moveCount <= 5)
    {
        baseDepth += 2;
    }
    else if (moveCount <= 15)
    {
    }
    else if (moveCount <= 30)
    {
        baseDepth -= 1;
    }
    else
    {
        baseDepth -= 1;
    }

    const int eval = Evaluation::evaluate(board);

    const int absEval = std::abs(eval);

    if (absEval > 5000)
    {
        baseDepth += 1;
    }
    else if (absEval > 2000)
    {
    }
    else if (absEval < 500)
    {
        baseDepth -= 1;
    }

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