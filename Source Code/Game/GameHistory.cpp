#include "Game/GameHistory.h"

std::vector<uint64_t> GameHistory::positions;

void GameHistory::clear()
{
    positions.clear();
}

void GameHistory::pushPosition(uint64_t zobristKey)
{
    positions.push_back(zobristKey);
}

bool GameHistory::hasRepeatedThreeTimes(uint64_t zobristKey)
{
    int count = 0;

    for (uint64_t key : positions)
    {
        if (key == zobristKey)
        {
            count++;

            if (count >= 3)
            {
                return true;
            }
        }
    }

    return false;
}

int GameHistory::getPositionCount(uint64_t zobristKey)
{
    int count = 0;

    for (uint64_t key : positions)
    {
        if (key == zobristKey)
        {
            count++;
        }
    }

    return count;
}

