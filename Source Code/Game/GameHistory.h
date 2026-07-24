#pragma once

#include <cstdint>
#include <vector>

class GameHistory
{
public:

    static void clear();

    static void pushPosition(uint64_t zobristKey);

    static bool hasRepeatedThreeTimes(uint64_t zobristKey);

    static int getPositionCount(uint64_t zobristKey);

private:

    static std::vector<uint64_t> positions;
};

