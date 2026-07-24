#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>

#include "Foundation/Move.h"

enum class TTFlag
{
    Exact,
    Alpha,
    Beta
};

struct TTEntry
{
    uint64_t key = 0;

    int depth = -1;

    int score = 0;

    TTFlag flag = TTFlag::Exact;

    Move bestMove;
};

class TranspositionTable
{
public:

    static void initialize();

    static void clear();

    static void store(
        uint64_t key,
        int depth,
        int score,
        TTFlag flag,
        const Move& bestMove);

    static bool probe(
        uint64_t key,
        int depth,
        int alpha,
        int beta,
        int& score,
        Move& bestMove);

    static void resize(size_t megaBytes);

private:

    static std::vector<TTEntry> table;
};