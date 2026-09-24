#include "Foundation/Bitboards.h"

#include "Move/AttackTables.h"
#include "Move/MoveValidator.h"

Bitboards Bitboards::computeLight(const Board& board)
{
    Bitboards bb{};
    const ChessColor colors[2] = { ChessColor::White, ChessColor::Black };
    const Piece pawnPieces[2] = { Piece::WhitePawn, Piece::BlackPawn };
    const Piece knightPieces[2] = { Piece::WhiteKnight, Piece::BlackKnight };
    const Piece bishopPieces[2] = { Piece::WhiteBishop, Piece::BlackBishop };
    const Piece rookPieces[2] = { Piece::WhiteRook, Piece::BlackRook };
    const Piece queenPieces[2] = { Piece::WhiteQueen, Piece::BlackQueen };
    const Piece kingPieces[2] = { Piece::WhiteKing, Piece::BlackKing };

    bb.allOccupied = board.getAllOccupancy();
    for (int c = 0; c < 2; ++c)
    {
        bb.pawns[c] = board.getBitboard(pawnPieces[c]);
        bb.knights[c] = board.getBitboard(knightPieces[c]);
        bb.bishops[c] = board.getBitboard(bishopPieces[c]);
        bb.rooks[c] = board.getBitboard(rookPieces[c]);
        bb.queens[c] = board.getBitboard(queenPieces[c]);
        bb.kings[c] = board.getBitboard(kingPieces[c]);
        bb.occupied[c] = board.getOccupancy(colors[c]);
    }
    return bb;
}


Bitboards Bitboards::computeHeavy(const Board& board)
{
    Bitboards bb = computeLight(board);
    const ChessColor colors[2] = { ChessColor::White, ChessColor::Black };

    for (int c = 0; c < 2; ++c)
    {
        Bitboard pieces = bb.pawns[c];
        while (pieces)
        {
            const Square s = popLeastSignificantBit(pieces);
            bb.pawnAttacks[c] |= c == 0 ? AttackTables::whitePawnAttacks(s) : AttackTables::blackPawnAttacks(s);
        }
        pieces = bb.knights[c];
        while (pieces) bb.knightAttacks[c] |= AttackTables::knightAttacks(popLeastSignificantBit(pieces));
        pieces = bb.bishops[c];
        while (pieces) bb.bishopAttacks[c] |= AttackTables::bishopAttacks(popLeastSignificantBit(pieces), bb.allOccupied);
        pieces = bb.rooks[c];
        while (pieces) bb.rookAttacks[c] |= AttackTables::rookAttacks(popLeastSignificantBit(pieces), bb.allOccupied);
        pieces = bb.queens[c];
        while (pieces) bb.queenAttacks[c] |= AttackTables::queenAttacks(popLeastSignificantBit(pieces), bb.allOccupied);

        const MoveValidator::CheckInfo info = MoveValidator::computeCheckInfo(board, colors[c]);
        bb.checkers[c] = info.checkers;
        bb.pinned[c] = info.pinned;
    }
    return bb;
}

Bitboards Bitboards::compute(const Board& board, bool withCheckInfo)
{
    if (!withCheckInfo) return computeLight(board);
    return computeHeavy(board);
}
