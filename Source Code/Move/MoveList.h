#pragma once

#include "Foundation/Move.h"

#include <array>
#include <cstddef>

class MoveList
{
public:
    static constexpr int Capacity = 256;

    void add(const Move& move)
    {
        if (count < Capacity)
            moves[static_cast<std::size_t>(count++)] = move;
    }

    void clear() { count = 0; }
    int size() const { return count; }

    const Move& operator[](int index) const
    {
        return moves[static_cast<std::size_t>(index)];
    }

    Move& operator[](int index)
    {
        return moves[static_cast<std::size_t>(index)];
    }

    Move* begin() { return moves.data(); }
    Move* end() { return moves.data() + count; }
    const Move* begin() const { return moves.data(); }
    const Move* end() const { return moves.data() + count; }

private:
    std::array<Move, Capacity> moves{};
    int count = 0;
};
