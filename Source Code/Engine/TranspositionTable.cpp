#include "Engine/TranspositionTable.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <new>

#if defined(_WIN32)
#include <malloc.h>
#endif

namespace
{
    constexpr size_t EntrySize = sizeof(TranspositionTable::Entry);
}

TranspositionTable::Entry* TranspositionTable::table = nullptr;
size_t TranspositionTable::tableSize = 0;
size_t TranspositionTable::mask = 0;
uint8_t TranspositionTable::currentAge = 0;

void TranspositionTable::initialize(size_t sizeMB)
{
    if (table)
    {
        std::free(table);
    }

    const size_t sizeBytes = sizeMB * 1024 * 1024;
    const size_t numEntries = sizeBytes / EntrySize;

    size_t powerOfTwo = 1;
    while (powerOfTwo < numEntries)
    {
        powerOfTwo <<= 1;
    }

    tableSize = powerOfTwo;
    mask = tableSize - 1;

#if defined(_WIN32)
    table = static_cast<Entry*>(_aligned_malloc(tableSize * EntrySize, 64));
#else
    table = static_cast<Entry*>(std::aligned_alloc(64, tableSize * EntrySize));
#endif
    if (!table)
    {
        std::cerr << "Failed to allocate transposition table (" << sizeMB << " MB)" << std::endl;
        std::exit(1);
    }

    clear();
    std::cout << "Transposition table initialized: " << sizeMB << " MB (" << tableSize << " entries)" << std::endl;
}

void TranspositionTable::clear()
{
    if (table)
    {
        for (size_t i = 0; i < tableSize; ++i)
        {
            table[i] = Entry{};
        }
    }
}

void TranspositionTable::newSearch()
{
    currentAge++;
    if (currentAge == 0)
    {
        // Age wrapped around, clear the table to avoid stale entries
        clear();
    }
}

TranspositionTable::Entry* TranspositionTable::probe(uint64_t key)
{
    if (!table) return nullptr;

    const size_t index = key & mask;
    Entry* entry = &table[index];

    if (entry->key == key)
    {
        return entry;
    }

    return nullptr;
}

void TranspositionTable::store(uint64_t key, int depth, int score, NodeType type, const Move& bestMove, const Bitboards& bitboards)
{
    if (!table) return;

    const size_t index = key & mask;
    Entry* entry = &table[index];

    if (entry->key == key)
    {
        if (depth >= entry->depth)
        {
            entry->depth = depth;
            entry->score = score;
            entry->type = type;
            entry->bestMove = bestMove;
            entry->bitboards = bitboards;
            entry->age = currentAge;
        }
    }
    else if (entry->key == 0 || currentAge > entry->age || depth >= entry->depth)
    {
        entry->key = key;
        entry->depth = depth;
        entry->score = score;
        entry->type = type;
        entry->bestMove = bestMove;
        entry->bitboards = bitboards;
        entry->age = currentAge;
    }
}

size_t TranspositionTable::size()
{
    return tableSize * EntrySize;
}

size_t TranspositionTable::entriesCount()
{
    return tableSize;
}