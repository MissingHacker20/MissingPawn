#include "Engine/Search.h"
#include "Engine/TranspositionTable.h"
#include "Foundation/Bitboards.h"
#include "Foundation/Board.h"
#include "Game/Fen.h"
#include "Move/AttackTables.h"
#include "Move/MoveGenerator.h"
#include "Move/MoveValidator.h"
#include "Others/TimeManager.h"
#include "Rate/Evaluation.h"
#include "Rate/Queen.h"

#include <iostream>
#include <string>

namespace
{
    bool containsMove(const MoveList& moves, const std::string& uci)
    {
        for (int i = 0; i < moves.size(); ++i)
            if (moves[i].toUCI() == uci) return true;
        return false;
    }

    bool testEnPassantGeneration()
    {
        Board board;
        if (!Fen::load(board, "8/8/8/3pP3/8/8/8/K6k w - d6 0 1")) return false;
        MoveList allMoves, captures;
        MoveGenerator::generateMoves(board, allMoves, {});
        MoveGenerator::generateCaptures(board, captures, {});
        if (!containsMove(allMoves, "e5d6") || !containsMove(captures, "e5d6"))
        {
            std::cerr << "FAIL en passant must be generated in full and capture lists\n";
            return false;
        }

        Board tactical;
        if (!Fen::load(tactical, "7k/8/8/3pP3/8/8/8/K7 w - d6 0 1")) return false;
        Move ep(Square::E5, Square::D6, Piece::WhitePawn, MoveFlag::EnPassant, Piece::BlackPawn);
        const int epSee = MoveValidator::see(tactical, ep);
        if (epSee <= 0)
        {
            std::cerr << "FAIL SEE must account for the captured pawn in en passant, score=" << epSee << '\n';
            return false;
        }

        Board pinned;
        if (!Fen::load(pinned, "k3r3/8/8/3pP3/8/8/8/4K3 w - d6 0 1")) return false;
        MoveGenerator::generateMoves(pinned, allMoves, {});
        MoveGenerator::generateCaptures(pinned, captures, {});
        if (containsMove(allMoves, "e5d6") || containsMove(captures, "e5d6"))
        {
            std::cerr << "FAIL illegal discovered-check en passant was generated\n";
            return false;
        }
        std::cout << "PASS en passant generation and king safety\n";
        return true;
    }

    bool testSee()
    {
        Board board;
        if (!Fen::load(board, "6k1/8/8/8/8/8/2r5/1B5K w - - 0 1")) return false;
        Move rookCapture(Square::B1, Square::C2, Piece::WhiteBishop, MoveFlag::Capture, Piece::BlackRook);
        const int cleanCaptureScore = MoveValidator::see(board, rookCapture);
        if (cleanCaptureScore != 5000)
        {
            std::cerr << "FAIL SEE expected 5000, got " << cleanCaptureScore << std::endl;
            return false;
        }

        if (!Fen::load(board, "6k1/8/8/8/8/2p5/1r6/2B4K w - - 0 1")) return false;
        Move losingCapture(Square::C1, Square::B2, Piece::WhiteBishop, MoveFlag::Capture, Piece::BlackRook);
        const int losingScore = MoveValidator::see(board, losingCapture);
        if (losingScore != 1700)
        {
            std::cerr << "FAIL SEE expected 1700 after pawn recapture, got " << losingScore << std::endl;
            return false;
        }

        if (!Fen::load(board, "3k4/8/8/8/8/3p4/2r5/1B1R3K w - - 0 1")) return false;
        Move pinnedRecapture(Square::B1, Square::C2, Piece::WhiteBishop, MoveFlag::Capture, Piece::BlackRook);
        const int pinnedScore = MoveValidator::see(board, pinnedRecapture);
        if (pinnedScore != 5000)
        {
            std::cerr << "FAIL SEE counted an illegal pinned pawn recapture, got " << pinnedScore << std::endl;
            return false;
        }
        if (MoveValidator::see(board, Square::C2) != 5000)
        {
            std::cerr << "FAIL position-level SEE counted an illegal pinned pawn recapture" << std::endl;
            return false;
        }

        if (!Fen::load(board, "7k/8/8/3pP3/8/8/8/K7 w - d6 0 1")) return false;
        Move epCapture(Square::E5, Square::D6, Piece::WhitePawn, MoveFlag::EnPassant, Piece::BlackPawn);
        if (MoveValidator::see(board, epCapture) != 1000)
        {
            std::cerr << "FAIL SEE did not account for the en-passant victim" << std::endl;
            return false;
        }

        std::cout << "PASS concrete-move SEE\n";
        return true;
    }

    bool testLazyEvaluation()
    {
        Board board;
        if (!Fen::load(board, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1")) return false;
        board.recomputeEval();
        const int before = board.getEvalScore();
        MoveList moves;
        MoveGenerator::generateMoves(board, moves, {});
        const Move move = moves[0];
        UndoInfo undo;
        board.makeMove(move, undo);
        const int cached = board.getEvalScore();
        const int fullEval = Evaluation::evaluate(board);
        if (cached != fullEval)
        {
            std::cerr << "FAIL cached evaluation mismatch: " << cached << " versus " << fullEval << std::endl;
            return false;
        }
        board.undoMove(move, undo);
        if (board.getEvalScore() != before)
        {
            std::cerr << "FAIL undo did not restore evaluation" << std::endl;
            return false;
        }
        std::cout << "PASS lazy evaluation cache/undo\n";
        return true;
    }

    bool testEvaluationCacheInvalidation()
    {
        Board board;
        if (!Fen::load(board, "4k3/8/8/8/8/8/8/4K3 w - - 0 1")) return false;
        const int firstEval = board.getEvalScore();

        if (!Fen::load(board, "7k/8/8/8/8/8/8/KQ6 w - - 0 1")) return false;
        const int loadedEval = board.getEvalScore();
        if (loadedEval != Evaluation::evaluate(board) || loadedEval == firstEval)
        {
            std::cerr << "FAIL loading a new FEN reused a stale evaluation" << std::endl;
            return false;
        }

        board.removePiece(Piece::WhiteQueen, Square::B1);
        if (board.getEvalScore() != Evaluation::evaluate(board))
        {
            std::cerr << "FAIL removePiece did not invalidate evaluation" << std::endl;
            return false;
        }

        board.setPiece(Piece::WhiteQueen, Square::D4);
        if (board.getEvalScore() != Evaluation::evaluate(board))
        {
            std::cerr << "FAIL setPiece did not invalidate evaluation" << std::endl;
            return false;
        }
        std::cout << "PASS evaluation cache invalidation\n";
        return true;
    }

    bool testQueenCenterBonus()
    {
        Board center;
        Board edge;
        if (!Fen::load(center, "8/8/8/8/3Q4/8/8/K6k w - - 0 1") ||
            !Fen::load(edge, "8/8/8/8/Q7/8/8/K6k w - - 0 1"))
            return false;

        const auto centerScore = QueenEvaluation::evaluatePair(
            center, Bitboards::computeLight(center), ChessColor::White);
        const auto edgeScore = QueenEvaluation::evaluatePair(
            edge, Bitboards::computeLight(edge), ChessColor::White);
        if (centerScore.first <= edgeScore.first || centerScore.second <= edgeScore.second)
        {
            std::cerr << "FAIL queen center score should exceed edge score" << std::endl;
            return false;
        }
        std::cout << "PASS queen center bonus direction\n";
        return true;
    }

    bool testTranspositionReplacement()
    {
        TranspositionTable::initialize(1);
        TranspositionTable::clear();
        const Move deepMove(Square::E2, Square::E4, Piece::WhitePawn, MoveFlag::DoublePawnPush);
        const Move shallowMove(Square::D2, Square::D4, Piece::WhitePawn, MoveFlag::DoublePawnPush);
        TranspositionTable::store(0x12345678, 9, 250,
            TranspositionTable::NodeType::Exact, deepMove);
        TranspositionTable::store(0x12345678, 4, -30,
            TranspositionTable::NodeType::LowerBound, shallowMove);

        const auto* entry = TranspositionTable::probe(0x12345678);
        if (!entry || entry->depth != 9 || entry->score != 250 ||
            entry->type != TranspositionTable::NodeType::Exact ||
            !(entry->bestMove == shallowMove))
        {
            std::cerr << "FAIL shallower TT entry replaced deeper result" << std::endl;
            return false;
        }
        std::cout << "PASS transposition replacement depth policy\n";
        return true;
    }

    bool testRootMateAtFiftyMoves()
    {
        Board board;
        if (!Fen::load(board, "7k/6Q1/5K2/8/8/8/8/8 b - - 100 1"))
        {
            std::cerr << "FAIL unable to load checkmate-at-100 FEN\n";
            return false;
        }
        MoveList moves;
        MoveGenerator::generateMoves(board, moves, {});
        if (moves.size() != 0 || !MoveValidator::isKingInCheck(board, ChessColor::Black))
        {
            std::cerr << "FAIL mate test FEN is not checkmate" << std::endl;
            return false;
        }
        Search::clearTranspositionTable();
        TimeManager::reset();
        TimeManager::setInfinite(true);
        const Move best = Search::findBestMove(board, 1);
        if (best.from != Square::None)
        {
            std::cerr << "FAIL checkmated root should return no move" << std::endl;
            return false;
        }
        Board drawn;
        if (!Fen::load(drawn, "7k/8/8/8/8/8/8/K6R w - - 100 1"))
        {
            std::cerr << "FAIL unable to load legal-position-at-100 FEN\n";
            return false;
        }
        Search::clearTranspositionTable();
        const Move drawBest = Search::findBestMove(drawn, 1);
        if (drawBest.from == Square::None)
        {
            std::cerr << "FAIL legal moves must still be returned at the fifty-move claim threshold" << std::endl;
            return false;
        }
        std::cout << "PASS root mate/draw ordering and legal move at 50-move threshold\n";
        return true;
    }
}

int main()
{
    AttackTables::initAttackTables();
    bool success = true;
    success = testEnPassantGeneration() && success;
    success = testSee() && success;
    success = testLazyEvaluation() && success;
    success = testEvaluationCacheInvalidation() && success;
    success = testQueenCenterBonus() && success;
    success = testRootMateAtFiftyMoves() && success;
    success = testTranspositionReplacement() && success;
    return success ? 0 : 1;
}
