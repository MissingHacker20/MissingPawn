#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

class GameHistory
{
public:

    static void clear();

    static void pushPosition(uint64_t zobristKey);

    static bool hasRepeatedThreeTimes(uint64_t zobristKey);

    static int getPositionCount(uint64_t zobristKey);

    // Zwraca listę wszystkich pozycji (w kolejności wystąpienia) od
    // początku gry. Używane do śledzenia powtórzeń w gałęzi searchu.
    static const std::vector<uint64_t>& getPositions();

private:

    // Wektor utrzymuje kolejność pozycji (potrzebny do undo/cofania),
    // a mapa liczników pozwala błyskawicznie sprawdzić, ile razy dana
    // pozycja wystąpiła (zamiast liniowego przeszukiwania wektora).
    static std::vector<uint64_t> positions;
    static std::unordered_map<uint64_t, int> positionCounts;
};
