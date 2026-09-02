#include "Rate/PawnStructure.h"

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

    // Figury przeciwnika bez pionków - wprost z bitboardów
    const Bitboard enemySlidersKnights = bitboards.knights[enemyIdx]
                                       | bitboards.bishops[enemyIdx]
                                       | bitboards.rooks[enemyIdx]
                                       | bitboards.queens[enemyIdx]
                                       | bitboards.kings[enemyIdx];

    // Occupancy bez własnych pionków
    const Bitboard occWithoutOwnPawns = bitboards.allOccupied & ~bitboards.pawns[ownIdx];

    Bitboard enemyPieces = enemySlidersKnights;
    while (enemyPieces)
    {
        const Square sq = popLeastSignificantBit(enemyPieces);
        const Bitboard withoutOwnPawns = AttackTables::pieceAttacks(board.pieceAt(sq), sq, occWithoutOwnPawns);
        const Bitboard withOwnPawns = AttackTables::pieceAttacks(board.pieceAt(sq), sq, bitboards.allOccupied);
        score += countBits(withoutOwnPawns & ~withOwnPawns);
    }

    return score;
}

int PawnStructureEvaluation::PawnFear(const Board& board, const Bitboards& bitboards, ChessColor color)
{
    const int ownIdx = Bitboards::indexOf(color);
    const int enemyIdx = 1 - ownIdx;

    // Unia ataków własnych pionków - gotowe pole z Bitboards
    const Bitboard ownPawnAttackUnion = bitboards.pawnAttacks[ownIdx];

    // Figury przeciwnika (bez pionków i króla, jak poprzednio brano wszystkie figury)
    Bitboard enemyPieces = bitboards.occupied[enemyIdx];

    int score = 0;
    while (enemyPieces)
    {
        const Square sq = popLeastSignificantBit(enemyPieces);
        const Bitboard attacks = AttackTables::pieceAttacks(board.pieceAt(sq), sq, bitboards.allOccupied);
        score += countBits(attacks & ownPawnAttackUnion);
    }

    return score;
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
