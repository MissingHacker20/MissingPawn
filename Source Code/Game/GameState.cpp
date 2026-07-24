#include "Game/GameState.h"

#include "Foundation/Board.h"
#include "Move/MoveGenerator.h"
#include "Move/MoveValidator.h"

GameResult GameState::getResult(const Board& board)
{
    if (board.getHalfmoveClock() >= 100)
    {
        return GameResult::FiftyMoveRule;
    }

    MoveList legalMoves;
    MoveGenerator::generateMoves(board, legalMoves);

    if (legalMoves.size() != 0)
    {
        return GameResult::Playing;
    }

    const ChessColor side = board.getSideToMove();
    if (!MoveValidator::isKingInCheck(board, side))
    {
        return GameResult::Stalemate;
    }

    return side == ChessColor::White
        ? GameResult::BlackWin
        : GameResult::WhiteWin;
}
