#include "Engine/Search.h"

#include <algorithm>
#include <limits>
#include <iostream>
#include <chrono>

#include "Game/GameState.h"
#include "Game/GameHistory.h"
#include "Engine/MoveOrdering.h"
#include "Engine/KillerMoves.h"
#include "Engine/HistoryHeuristic.h"
#include "Engine/Transposition.h"
#include "Move/MoveGenerator.h"
#include "Move/MoveValidator.h"
#include "Rate/Evaluation.h"
#include "Others/TimeManager.h"

namespace
{
constexpr int Infinity = std::numeric_limits<int>::max() / 2;

int nodesSearched = 0;
auto searchStartTime = std::chrono::steady_clock::time_point{};
int currentIterativeDepth = 0;
}

bool Search::ponderEnabled = false;

void Search::setPonder(bool enabled)
{
    ponderEnabled = enabled;
}

bool Search::isPondering()
{
    return ponderEnabled;
}

bool Search::shouldStopSearch()
{
    if (TimeManager::shouldStop())
    {
        return true;
    }

    if (ponderEnabled)
    {
        return false;
    }

    return false;
}

Move Search::findBestMove(Board& board, int depth)
{
    searchStartTime = std::chrono::steady_clock::now();

    Move bestMove;
    Move rootMove;

    //--------------------------------------------------
    // Iterative Deepening
    //--------------------------------------------------

    for (int currentDepth = 1; currentDepth <= depth; currentDepth++)
    {
        currentIterativeDepth = currentDepth;
        nodesSearched = 0;

        MoveList moves;
        MoveGenerator::generateMoves(board, moves);
        MoveOrdering::sortMoves(board, moves, currentDepth);

        if (moves.size() == 0)
        {
            break;
        }

        int bestScore = -Infinity;
        int alpha = -Infinity;
        int beta = Infinity;

        for (int index = 0; index < moves.size(); ++index)
        {
            if (shouldStopSearch())
            {
                break;
            }

            const Move& move = moves[index];
            UndoInfo undoInfo;
            board.makeMove(move, undoInfo);
            nodesSearched++;

            int score;

            if (index == 0)
            {
                score = -negamax(board, currentDepth - 1, -beta, -alpha, 1);
            }
            else
            {
                score = -negamax(board, currentDepth - 1, -alpha - 1, -alpha, 1);

                if (score > alpha && score < beta)
                {
                    score = -negamax(board, currentDepth - 1, -beta, -alpha, 1);
                }
            }

            board.undoMove(move, undoInfo);

            if (score > bestScore)
            {
                bestScore = score;
                bestMove = move;

                if (currentDepth == 1)
                {
                    rootMove = move;
                }
            }

            alpha = std::max(alpha, score);
        }

        //--------------------------------------------------
        // UCI info output
        //--------------------------------------------------

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - searchStartTime).count();

        uint64_t nps = (elapsed > 0)
            ? static_cast<uint64_t>(nodesSearched) * 1000 / static_cast<uint64_t>(elapsed)
            : 0;

        std::cout << "info"
                  << " depth " << currentDepth
                  << " score cp " << bestScore
                  << " nodes " << nodesSearched
                  << " nps " << nps
                  << " time " << elapsed
                  << " pv " << bestMove.toUCI();

        // Print principal variation
        Board pvBoard = board;
        Move pvMove = bestMove;
        for (int pvDepth = 0; pvDepth < 4; pvDepth++)
        {
            if (pvMove.from == Square::None)
                break;

            UndoInfo pvUndo;
            pvBoard.makeMove(pvMove, pvUndo);

            MoveList pvMoves;
            MoveGenerator::generateMoves(pvBoard, pvMoves);
            MoveOrdering::sortMoves(pvBoard, pvMoves, currentDepth - pvDepth - 1);

            if (pvMoves.size() > 0)
            {
                pvMove = pvMoves[0];
                std::cout << " " << pvMove.toUCI();
            }
            else
            {
                break;
            }
        }

        std::cout << std::endl;
    }

    // Store in history
    if (bestMove.from != Square::None)
    {
        HistoryHeuristic::add(board.getSideToMove(), bestMove, depth);
    }

    std::cout << "bestmove " << bestMove.toUCI() << std::endl;

    return bestMove;
}

int Search::quiesce(Board& board, int alpha, int beta, int ply)
{
    nodesSearched++;

    if (shouldStopSearch())
    {
        return 0;
    }

    int standPat = Evaluation::evaluate(board);
    if (board.getSideToMove() == ChessColor::Black)
    {
        standPat = -standPat;
    }

    if (standPat >= beta)
    {
        return beta;
    }

    if (standPat > alpha)
    {
        alpha = standPat;
    }

    MoveList captures;
    MoveGenerator::generateCaptures(board, captures);
    MoveOrdering::sortMoves(board, captures, 0);

    for (int i = 0; i < captures.size(); i++)
    {
        const Move& move = captures[i];

        // Delta pruning
        int pieceValue = 0;
        switch (move.capturedPiece)
        {
        case Piece::WhitePawn:   case Piece::BlackPawn:   pieceValue = 100;  break;
        case Piece::WhiteKnight: case Piece::BlackKnight: pieceValue = 320;  break;
        case Piece::WhiteBishop: case Piece::BlackBishop: pieceValue = 330;  break;
        case Piece::WhiteRook:   case Piece::BlackRook:   pieceValue = 500;  break;
        case Piece::WhiteQueen:  case Piece::BlackQueen:  pieceValue = 900;  break;
        default: pieceValue = 0; break;
        }

        if (standPat + pieceValue + 200 < alpha)
        {
            continue;
        }

        UndoInfo undoInfo;
        board.makeMove(move, undoInfo);

        const int score = -quiesce(board, -beta, -alpha, ply + 1);

        board.undoMove(move, undoInfo);

        if (score > alpha)
        {
            alpha = score;

            if (alpha >= beta)
            {
                break;
            }
        }
    }

    return alpha;
}

int Search::negamax(Board& board, int depth, int alpha, int beta, int ply)
{
    nodesSearched++;

    if (shouldStopSearch())
    {
        return 0;
    }

    //--------------------------------------------------
    // Check terminal conditions
    //--------------------------------------------------

    const int endScore = terminalScore(board, ply);
    if (endScore != 0)
    {
        return endScore;
    }

    // Threefold repetition
    if (GameHistory::hasRepeatedThreeTimes(board.getZobristKey()))
    {
        return 0;
    }

    //--------------------------------------------------
    // Transposition table lookup
    //--------------------------------------------------

    Move ttBestMove;
    int ttScore = 0;

    if (TranspositionTable::probe(
            board.getZobristKey(),
            depth,
            alpha,
            beta,
            ttScore,
            ttBestMove))
    {
        return ttScore;
    }

    //--------------------------------------------------
    // Quiescence search at leaf nodes
    //--------------------------------------------------

    if (depth == 0)
    {
        int score = quiesce(board, alpha, beta, ply);
        return score;
    }

    //--------------------------------------------------
    // Null move pruning
    //--------------------------------------------------

    if (depth >= 3 && ply > 0)
    {
        UndoInfo nullUndo;
        board.makeNullMove(nullUndo);

        int nullScore = -negamax(board, depth - 3, -beta, -beta + 1, ply + 1);

        board.undoNullMove(nullUndo);

        if (nullScore >= beta)
        {
            return beta;
        }
    }

    //--------------------------------------------------
    // Generate and search moves
    //--------------------------------------------------

    MoveList moves;
    MoveGenerator::generateMoves(board, moves);
    MoveOrdering::sortMoves(board, moves, depth);

    if (moves.size() == 0)
    {
        const GameResult result = GameState::getResult(board);
        if (result == GameResult::Stalemate)
        {
            return 0;
        }
        return -Infinity + ply;
    }

    Move bestMove;
    int bestScore = -Infinity;
    int movesSearched = 0;

    for (int index = 0; index < moves.size(); ++index)
    {
        const Move& move = moves[index];

        // Late Move Reduction
        bool doReduction = false;
        int reduction = 0;

        if (index >= 3 && depth >= 3 &&
            move.capturedPiece == Piece::None &&
            move.flag != MoveFlag::KingCastle &&
            move.flag != MoveFlag::QueenCastle &&
            !(move.flag >= MoveFlag::PromotionKnight &&
              move.flag <= MoveFlag::PromotionCaptureQueen))
        {
            doReduction = true;
            reduction = 1;

            if (index >= 6 && depth >= 4)
            {
                reduction = 2;
            }
        }

        UndoInfo undoInfo;
        board.makeMove(move, undoInfo);

        int score;

        if (doReduction)
        {
            score = -negamax(board, depth - 1 - reduction, -alpha - 1, -alpha, ply + 1);

            if (score > alpha)
            {
                score = -negamax(board, depth - 1, -alpha - 1, -alpha, ply + 1);
            }
        }
        else
        {
            score = -negamax(board, depth - 1, -alpha - 1, -alpha, ply + 1);
        }

        if (score > alpha && score < beta)
        {
            score = -negamax(board, depth - 1, -beta, -alpha, ply + 1);
        }

        board.undoMove(move, undoInfo);

        if (score > bestScore)
        {
            bestScore = score;
            bestMove = move;
        }

        movesSearched++;
        alpha = std::max(alpha, score);

        if (alpha >= beta)
        {
            // Store killer moves
            if (move.capturedPiece == Piece::None &&
                move.flag != MoveFlag::KingCastle &&
                move.flag != MoveFlag::QueenCastle &&
                !(move.flag >= MoveFlag::PromotionKnight &&
                  move.flag <= MoveFlag::PromotionCaptureQueen))
            {
                KillerMoves::add(depth, move);
                HistoryHeuristic::add(board.getSideToMove(), move, depth);
            }

            break;
        }
    }

    //--------------------------------------------------
    // Store in transposition table
    //--------------------------------------------------

    TTFlag flag = TTFlag::Exact;

    if (bestScore <= alpha)
    {
        flag = TTFlag::Alpha;
    }
    else if (bestScore >= beta)
    {
        flag = TTFlag::Beta;
    }

    TranspositionTable::store(
        board.getZobristKey(),
        depth,
        bestScore,
        flag,
        bestMove);

    return bestScore;
}

int Search::terminalScore(const Board& board, int ply)
{
    const GameResult result = GameState::getResult(board);

    if (result == GameResult::Playing)
    {
        return 0;
    }

    if (result == GameResult::Stalemate ||
        result == GameResult::FiftyMoveRule ||
        result == GameResult::ThreefoldRepetition)
    {
        return 0;
    }

    const ChessColor winner = result == GameResult::WhiteWin
        ? ChessColor::White
        : ChessColor::Black;

    return winner == board.getSideToMove()
        ? MateScore - ply
        : -MateScore + ply;
}
