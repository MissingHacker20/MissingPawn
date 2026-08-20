#pragma once

#include <cstdint>

#include "Foundation/Piece.h"
#include "Foundation/Square.h"
#include "Foundation/Color.h"
#include "Foundation/Board.h"

namespace Zobrist
{
void initializeTables();

uint64_t getPieceKey(Piece piece, Square square);
uint64_t getSideKey(ChessColor side);
uint64_t getCastlingKey(uint8_t rights);
uint64_t getEnPassantKey(Square square);

bool isEnPassantValid(const Board& board, Square epSquare);

uint64_t calculateHash(const Board& board);
}
