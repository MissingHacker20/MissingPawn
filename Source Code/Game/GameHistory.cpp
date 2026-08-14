#include "Game/GameHistory.h"

std::vector<uint64_t> GameHistory::positions;
std::unordered_map<uint64_t, int> GameHistory::positionCounts;

void GameHistory::clear()
{
    positions.clear();
    positionCounts.clear();
}

void GameHistory::pushPosition(uint64_t zobristKey)
{
    positions.push_back(zobristKey);
    positionCounts[zobristKey]++;
}

bool GameHistory::hasRepeatedThreeTimes(uint64_t zobristKey)
{
    auto it = positionCounts.find(zobristKey);
    return it != positionCounts.end() && it->second >= 3;
}

int GameHistory::getPositionCount(uint64_t zobristKey)
{
    auto it = positionCounts.find(zobristKey);
    return (it != positionCounts.end()) ? it->second : 0;
}

const std::vector<uint64_t>& GameHistory::getPositions()
{
    return positions;
}
