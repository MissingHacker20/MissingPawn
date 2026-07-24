#pragma once

#include "Foundation/Move.h"

#include <vector>

class MoveList
{
public:
    void add(const Move& move)
    {
        moves.push_back(move);
    }

    void clear()
    {
        moves.clear();
    }

    int size() const
    {
        return static_cast<int>(moves.size());
    }

    const Move& operator[](int index) const
    {
        return moves[static_cast<std::size_t>(index)];
    }

    Move& operator[](int index)
    {
        return moves[static_cast<std::size_t>(index)];
    }

    std::vector<Move>::iterator begin()
    {
        return moves.begin();
    }

    std::vector<Move>::iterator end()
    {
        return moves.end();
    }

    std::vector<Move>::const_iterator begin() const
    {
        return moves.begin();
    }

    std::vector<Move>::const_iterator end() const
    {
        return moves.end();
    }

private:
    std::vector<Move> moves;
};
