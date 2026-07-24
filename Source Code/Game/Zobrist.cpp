#include "Game/Zobrist.h"

#include "Foundation/Board.h"

namespace
{
uint64_t mix(uint64_t value)
{
    value ^= value >> 30;
    value *= 0xbf58476d1ce4e5b9ULL;
    value ^= value >> 27;
    value *= 0x94d049bb133111ebULL;
    return value ^ (value >> 31);
}
}

namespace Zobrist
{
uint64_t calculateHash(const Board& board)
{
    uint64_t hash = 0x9e3779b97f4a7c15ULL;

    for (int square = 0; square < 64; ++square)
    {
        const Piece piece = board.pieceAt(static_cast<Square>(square));
        hash ^= mix(static_cast<uint64_t>(square + 1) *
                    (static_cast<uint64_t>(piece) + 1));
    }

    hash ^= mix(static_cast<uint64_t>(board.getSideToMove() == ChessColor::White));
    hash ^= mix(static_cast<uint64_t>(board.getCastlingRights()) + 0x100ULL);
    hash ^= mix(static_cast<uint64_t>(board.getEnPassantSquare()) + 0x200ULL);

    return hash;
}
}
