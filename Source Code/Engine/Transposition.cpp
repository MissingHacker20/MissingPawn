#include "Transposition.h"

#include <algorithm>

std::vector<TTEntry> TranspositionTable::table(
    1 << 20);

void TranspositionTable::initialize()
{
    clear();
}

void TranspositionTable::clear()
{
    for (auto& entry : table)
    {
        entry.depth = -1;
        entry.key = 0;
    }
}

void TranspositionTable::store(
    uint64_t key,
    int depth,
    int score,
    TTFlag flag,
    const Move& bestMove)
{
    if (table.empty())
    {
        initialize();
    }

    TTEntry& entry =
        table[key % table.size()];

    if (entry.depth > depth)
    {
        return;
    }

    entry.key = key;
    entry.depth = depth;
    entry.score = score;
    entry.flag = flag;
    entry.bestMove = bestMove;
}

bool TranspositionTable::probe(
    uint64_t key,
    int depth,
    int alpha,
    int beta,
    int& score,
    Move& bestMove)
{
    if (table.empty())
    {
        return false;
    }

    TTEntry& entry =
        table[key % table.size()];

    if (entry.key != key)
    {
        return false;
    }

    bestMove = entry.bestMove;

    if (entry.depth < depth)
    {
        return false;
    }

    switch (entry.flag)
    {
    case TTFlag::Exact:

        score = entry.score;
        return true;

    case TTFlag::Alpha:

        if (entry.score <= alpha)
        {
            score = alpha;
            return true;
        }

        break;

    case TTFlag::Beta:

        if (entry.score >= beta)
        {
            score = beta;
            return true;
        }

        break;
    }

    return false;
}

void TranspositionTable::resize(size_t megaBytes)
{
    constexpr size_t BytesPerMB =
        1024ull * 1024ull;

    const size_t bytes = megaBytes * BytesPerMB;
    const size_t entries = std::max<size_t>(
        1,
        bytes / sizeof(TTEntry));

    table.resize(entries);
    clear();
}