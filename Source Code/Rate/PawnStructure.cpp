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

int PawnStructureEvaluation::PieceSpace(const Board& board, const Bitboards& /*bitboards*/, ChessColor color)
{
    // UWAGA: opiera się na cache legalnych ruchów (Board::getPieceMoves),
    // którego nie da się wyrazić statycznymi bitboardami. Patrz komentarz
    // w podsumowaniu zmian.
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
