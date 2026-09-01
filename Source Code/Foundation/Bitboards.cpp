#include "Foundation/Bitboards.h"

#include "Move/AttackTables.h"
#include "Move/MoveValidator.h"

Bitboards Bitboards::compute(const Board& board, bool withCheckInfo)
{
    Bitboards bb{};

    const ChessColor colors[2] = { ChessColor::White, ChessColor::Black };
    const Piece pawnPieces[2]   = { Piece::WhitePawn,   Piece::BlackPawn };
    const Piece knightPieces[2] = { Piece::WhiteKnight, Piece::BlackKnight };
    const Piece bishopPieces[2] = { Piece::WhiteBishop, Piece::BlackBishop };
    const Piece rookPieces[2]   = { Piece::WhiteRook,   Piece::BlackRook };
    const Piece queenPieces[2]  = { Piece::WhiteQueen,  Piece::BlackQueen };
    const Piece kingPieces[2]   = { Piece::WhiteKing,   Piece::BlackKing };

    bb.allOccupied = board.getAllOccupancy();

    for (int c = 0; c < 2; ++c)
    {
        // Figury - bezpośrednio z Board
        bb.pawns[c]   = board.getBitboard(pawnPieces[c]);
        bb.knights[c] = board.getBitboard(knightPieces[c]);
        bb.bishops[c] = board.getBitboard(bishopPieces[c]);
        bb.rooks[c]   = board.getBitboard(rookPieces[c]);
        bb.queens[c]  = board.getBitboard(queenPieces[c]);
        bb.kings[c]   = board.getBitboard(kingPieces[c]);

        bb.occupied[c] = board.getOccupancy(colors[c]);

        // Unie ataków - z gotowych tablic AttackTables
        Bitboard bb2 = bb.pawns[c];
        while (bb2)
        {
            const Square s = popLeastSignificantBit(bb2);
            bb.pawnAttacks[c] |= (c == 0)
                ? AttackTables::whitePawnAttacks(s)
                : AttackTables::blackPawnAttacks(s);
        }

        bb2 = bb.knights[c];
        while (bb2)
        {
            const Square s = popLeastSignificantBit(bb2);
            bb.knightAttacks[c] |= AttackTables::knightAttacks(s);
        }

        bb2 = bb.bishops[c];
        while (bb2)
        {
            const Square s = popLeastSignificantBit(bb2);
            bb.bishopAttacks[c] |= AttackTables::bishopAttacks(s, bb.allOccupied);
        }

        bb2 = bb.rooks[c];
        while (bb2)
        {
            const Square s = popLeastSignificantBit(bb2);
            bb.rookAttacks[c] |= AttackTables::rookAttacks(s, bb.allOccupied);
        }

        bb2 = bb.queens[c];
        while (bb2)
        {
            const Square s = popLeastSignificantBit(bb2);
            bb.queenAttacks[c] |= AttackTables::queenAttacks(s, bb.allOccupied);
        }

        }

        // Szachy i przypięcia - z gotowego MoveValidator::computeCheckInfo
        if (withCheckInfo)
        {
            for (int c = 0; c < 2; ++c)
            {
                const MoveValidator::CheckInfo info =
                    MoveValidator::computeCheckInfo(board, colors[c]);

                bb.checkers[c] = info.checkers;
                bb.pinned[c] = info.pinned;
            }
        }

        return bb;
    }
