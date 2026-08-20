#include "Game/GameState.h"

#include "Foundation/Board.h"
#include "Game/GameHistory.h"
#include "Move/MoveGenerator.h"
#include "Move/MoveValidator.h"

GameResult GameState::getResult(const Board& board)
{
    if (board.getHalfmoveClock() >= 100)
    {
        return GameResult::FiftyMoveRule;
    }

    if (GameHistory::hasRepeatedThreeTimes(board.getZobristKey()))
    {
        return GameResult::ThreefoldRepetition;
    }

    // generateMoves wymaga nie-const Board& (do sprawdzania legalności przez
    // makeMove/undoMove). Ponieważ getResult jest const, używamy lokalnej kopii.
    Board copy = board;
    MoveList legalMoves;
    const MoveValidator::CheckInfo pseudoInfo{};
    MoveGenerator::generateMoves(copy, legalMoves, pseudoInfo);
    MoveValidator::filterLegalMoves(copy, legalMoves);

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
