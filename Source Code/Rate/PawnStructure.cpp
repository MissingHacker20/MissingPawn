#include "Rate/PawnStructure.h"

#include <algorithm>

#include "Foundation/Bitboard.h"
#include "Foundation/Bitboards.h"
#include "Foundation/Board.h"
#include "Move/AttackTables.h"

namespace
{

inline Piece pawnFor(ChessColor color)
{
    return color == ChessColor::White ? Piece::WhitePawn : Piece::BlackPawn;
}

}

int PawnStructureEvaluation::PieceBlocker(const Board& board, const Bitboards& bitboards, ChessColor color)
{
    const int ownIdx = Bitboards::indexOf(color);
    const int enemyIdx = 1 - ownIdx;

    int score = 0;

    // A blocker is valuable only when it actually occupies a square that
    // matters to an enemy piece. Do not count every square behind a pawn.
    const Bitboard enemyPieces = bitboards.occupied[enemyIdx]
        & ~bitboards.pawns[enemyIdx]
        & ~bitboards.kings[enemyIdx];
    const Bitboard ownPawns = bitboards.pawns[ownIdx];

    Bitboard pieces = enemyPieces;
    while (pieces)
    {
        const Square sq = popLeastSignificantBit(pieces);
        const Bitboard attacks = AttackTables::pieceAttacks(
            board.pieceAt(sq), sq, bitboards.allOccupied);
        // One small bonus per pawn that restricts the piece, capped per piece.
        score += std::min(3, countBits(attacks & ownPawns));
    }

    return std::min(score, 24);
}

int PawnStructureEvaluation::PawnFear(const Board& board, const Bitboards& bitboards, ChessColor color)
{
    const int ownIdx = Bitboards::indexOf(color);
    const int enemyIdx = 1 - ownIdx;

    // Unia ataków własnych pionków - gotowe pole z Bitboards
    const Bitboard ownPawnAttackUnion = bitboards.pawnAttacks[ownIdx];

    // Fear measures restricted enemy pieces, not the number of attacked empty
    // squares. Kings and pawns are excluded: they do not represent useful
    // mobility targets for this heuristic.
    Bitboard enemyPieces = bitboards.occupied[enemyIdx]
        & ~bitboards.pawns[enemyIdx]
        & ~bitboards.kings[enemyIdx];

    int score = 0;
    while (enemyPieces)
    {
        const Square sq = popLeastSignificantBit(enemyPieces);
        if (getBit(ownPawnAttackUnion, sq))
        {
            const Piece piece = board.pieceAt(sq);
            const int weight = (piece == Piece::WhiteQueen || piece == Piece::BlackQueen)
                ? 3 : (piece == Piece::WhiteRook || piece == Piece::BlackRook ? 2 : 1);
            score += weight;
        }
    }

    return std::min(score, 24);
}

int PawnStructureEvaluation::PieceSpace(const Board& /*board*/, const Bitboards& bitboards, ChessColor color)
{
    const int ownIdx = Bitboards::indexOf(color);
    const int enemyIdx = 1 - ownIdx;
    const Bitboard ownAttacks = bitboards.pawnAttacks[ownIdx]
        | bitboards.knightAttacks[ownIdx]
        | bitboards.bishopAttacks[ownIdx]
        | bitboards.rookAttacks[ownIdx]
        | bitboards.queenAttacks[ownIdx];
    const Bitboard enemyAttacks = bitboards.pawnAttacks[enemyIdx]
        | bitboards.knightAttacks[enemyIdx]
        | bitboards.bishopAttacks[enemyIdx]
        | bitboards.rookAttacks[enemyIdx]
        | bitboards.queenAttacks[enemyIdx];

    // Space is controlled territory in the opponent's half, not legal-move
    // count.  Only uncontested central territory contributes to the score.
    Bitboard enemyHalf = 0;
    const int firstEnemyRank = color == ChessColor::White ? 4 : 0;
    const int lastEnemyRank = color == ChessColor::White ? 7 : 3;
    for (int rank = firstEnemyRank; rank <= lastEnemyRank; ++rank)
    {
        enemyHalf |= 0xFFULL << (rank * 8);
    }

    constexpr Bitboard CenterFiles = 0x3C3C3C3C3C3C3C3CULL;
    const Bitboard space = ownAttacks & ~enemyAttacks & enemyHalf & CenterFiles;
    return countBits(space) * 2;
}

int PawnStructureEvaluation::evaluate(const Board& board, const Bitboards& bitboards, ChessColor color)
{
    return PieceBlocker(board, bitboards, color)
         + PawnFear(board, bitboards, color)
         + PieceSpace(board, bitboards, color);
}

Bitboard PawnStructureEvaluation::getAffectedPawns(const Board& board, const Move& move)
{
    (void)board;
    (void)move;
    return 0;
}
