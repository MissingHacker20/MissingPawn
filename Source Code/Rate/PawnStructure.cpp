#include "Rate/PawnStructure.h"

#include "Foundation/Bitboard.h"
#include "Foundation/Board.h"
#include "Move/AttackTables.h"

namespace
{

inline Bitboard computePieceAttacks(const Board& board, Square sq)
{
    const Piece piece = board.pieceAt(sq);

    switch (piece)
    {
    case Piece::WhitePawn:
        return AttackTables::whitePawnAttacks(sq);

    case Piece::BlackPawn:
        return AttackTables::blackPawnAttacks(sq);

    case Piece::WhiteKnight:
    case Piece::BlackKnight:
        return AttackTables::knightAttacks(sq);

    case Piece::WhiteKing:
    case Piece::BlackKing:
        return AttackTables::kingAttacks(sq);

    case Piece::WhiteBishop:
    case Piece::BlackBishop:
    {
        Bitboard attacks = 0;
        constexpr int dirs[4][2] = {{1,1}, {1,-1}, {-1,1}, {-1,-1}};
        const Bitboard occ = board.getAllOccupancy();
        for (int d = 0; d < 4; ++d)
        {
            int f = static_cast<int>(sq) % 8 + dirs[d][0];
            int r = static_cast<int>(sq) / 8 + dirs[d][1];
            while (f >= 0 && f < 8 && r >= 0 && r < 8)
            {
                const Square target = static_cast<Square>(r * 8 + f);
                setBit(attacks, target);
                if (getBit(occ, target))
                {
                    break;
                }
                f += dirs[d][0];
                r += dirs[d][1];
            }
        }
        return attacks;
    }

    case Piece::WhiteRook:
    case Piece::BlackRook:
    {
        Bitboard attacks = 0;
        constexpr int dirs[4][2] = {{1,0}, {-1,0}, {0,1}, {0,-1}};
        const Bitboard occ = board.getAllOccupancy();
        for (int d = 0; d < 4; ++d)
        {
            int f = static_cast<int>(sq) % 8 + dirs[d][0];
            int r = static_cast<int>(sq) / 8 + dirs[d][1];
            while (f >= 0 && f < 8 && r >= 0 && r < 8)
            {
                const Square target = static_cast<Square>(r * 8 + f);
                setBit(attacks, target);
                if (getBit(occ, target))
                {
                    break;
                }
                f += dirs[d][0];
                r += dirs[d][1];
            }
        }
        return attacks;
    }

    case Piece::WhiteQueen:
    case Piece::BlackQueen:
    {
        Bitboard attacks = 0;
        constexpr int dirs[8][2] = {{1,0}, {-1,0}, {0,1}, {0,-1}, {1,1}, {1,-1}, {-1,1}, {-1,-1}};
        const Bitboard occ = board.getAllOccupancy();
        for (int d = 0; d < 8; ++d)
        {
            int f = static_cast<int>(sq) % 8 + dirs[d][0];
            int r = static_cast<int>(sq) / 8 + dirs[d][1];
            while (f >= 0 && f < 8 && r >= 0 && r < 8)
            {
                const Square target = static_cast<Square>(r * 8 + f);
                setBit(attacks, target);
                if (getBit(occ, target))
                {
                    break;
                }
                f += dirs[d][0];
                r += dirs[d][1];
            }
        }
        return attacks;
    }

    default:
        return 0;
    }
}

inline Piece pawnFor(ChessColor color)
{
    return color == ChessColor::White ? Piece::WhitePawn : Piece::BlackPawn;
}

}

int PawnStructureEvaluation::PieceBlocker(const Board& board, ChessColor color)
{
    const ChessColor enemy = (color == ChessColor::White) ? ChessColor::Black : ChessColor::White;
    const Piece ownPawn = (color == ChessColor::White) ? Piece::WhitePawn : Piece::BlackPawn;

    int score = 0;

    Board boardWithoutPawns = board;
    const Bitboard ownPawnBB = board.getBitboard(ownPawn);
    Bitboard temp = ownPawnBB;
    while (temp)
    {
        const Square sq = popLeastSignificantBit(temp);
        boardWithoutPawns.removePiece(ownPawn, sq);
    }

    Bitboard enemyPieces = boardWithoutPawns.getBitboard(
        (enemy == ChessColor::White) ? Piece::WhiteKnight : Piece::BlackKnight);
    enemyPieces |= boardWithoutPawns.getBitboard(
        (enemy == ChessColor::White) ? Piece::WhiteBishop : Piece::BlackBishop);
    enemyPieces |= boardWithoutPawns.getBitboard(
        (enemy == ChessColor::White) ? Piece::WhiteRook : Piece::BlackRook);
    enemyPieces |= boardWithoutPawns.getBitboard(
        (enemy == ChessColor::White) ? Piece::WhiteQueen : Piece::BlackQueen);
    enemyPieces |= boardWithoutPawns.getBitboard(
        (enemy == ChessColor::White) ? Piece::WhiteKing : Piece::BlackKing);

    while (enemyPieces)
    {
        const Square sq = popLeastSignificantBit(enemyPieces);
        const Bitboard withoutOwnPawns = computePieceAttacks(boardWithoutPawns, sq);
        const Bitboard withOwnPawns = board.getPieceAttacks(sq);
        score += countBits(withoutOwnPawns & ~withOwnPawns);
    }

    return score;
}

int PawnStructureEvaluation::PawnFear(const Board& board, ChessColor color)
{
    const Piece ownPawn = pawnFor(color);

    Bitboard ownPawnAttackUnion = 0;
    Bitboard ownPawns = board.getBitboard(ownPawn);
    while (ownPawns)
    {
        const Square sq = popLeastSignificantBit(ownPawns);
        ownPawnAttackUnion |= board.getPieceAttacks(sq);
    }

    int score = 0;
    const ChessColor enemy = (color == ChessColor::White) ? ChessColor::Black : ChessColor::White;

    for (int index = 0; index < 64; ++index)
    {
        const Square sq = static_cast<Square>(index);
        const Piece piece = board.pieceAt(sq);

        if (piece == Piece::None)
        {
            continue;
        }

        if (getPieceColor(piece) != enemy)
        {
            continue;
        }

        score += countBits(board.getPieceAttacks(sq) & ownPawnAttackUnion);
    }

    return score;
}

int PawnStructureEvaluation::PieceSpace(const Board& board, ChessColor color)
{
    int own = 0;
    int enemyTotal = 0;

    for (int index = 0; index < 64; ++index)
    {
        const Square sq = static_cast<Square>(index);
        const Piece piece = board.pieceAt(sq);

        if (piece == Piece::None)
        {
            continue;
        }

        const int moves = countBits(board.getPieceMoves(sq));

        if (getPieceColor(piece) == color)
        {
            own += moves;
        }
        else
        {
            enemyTotal += moves;
        }
    }

    return own - enemyTotal;
}

int PawnStructureEvaluation::evaluate(const Board& board, ChessColor color)
{
    return PieceBlocker(board, color)
         + PawnFear(board, color)
         + PieceSpace(board, color);
}

Bitboard PawnStructureEvaluation::getAffectedPawns(const Board& board, const Move& move)
{
    (void)board;
    (void)move;
    return 0;
}
