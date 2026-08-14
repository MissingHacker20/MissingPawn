#pragma once

#include <cstdint>

class Board;

namespace Zobrist
{
uint64_t calculateHash(const Board& board);
}
