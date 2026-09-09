#pragma once

#include <cstdint>
#include <cstddef>

#include "Foundation/Board.h"
#include "Foundation/Bitboards.h"
#include "Foundation/Move.h"

class TranspositionTable
{
public:
    enum class NodeType : uint8_t
    {
        Exact,
        LowerBound,
        UpperBound
    };

    struct Entry
    {
        uint64_t key = 0;
        int depth = 0;
        int score = 0;
        NodeType type = NodeType::Exact;
        Move bestMove{};
        Bitboards bitboards;
        uint8_t age = 0;
    };

    static void initialize(size_t sizeMB = 64);
    static void clear();
    static void newSearch();

    static Entry* probe(uint64_t key);
    static void store(uint64_t key, int depth, int score, NodeType type, const Move& bestMove, const Bitboards& bitboards);

    static size_t size();
    static size_t entriesCount();

private:
    static Entry* table;
    static size_t tableSize;
    static size_t mask;
    static uint8_t currentAge;
};