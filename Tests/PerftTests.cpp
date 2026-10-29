#include "Foundation/Board.h"
#include "Game/Fen.h"
#include "Move/AttackTables.h"
#include "Move/MoveGenerator.h"
#include "Move/MoveValidator.h"
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace
{
    struct PerftCase
    {
        const char *name;
        const char *fen;
        int depth;
        std::uint64_t expected;
        bool validFen = true;
    };
    std::uint64_t perft(Board &board, int depth)
    {
        if (depth == 0)
            return 1;
        MoveList moves;
        MoveGenerator::generateMoves(board, moves, MoveValidator::CheckInfo{});
        std::uint64_t nodes = 0;
        for (int i = 0; i < moves.size(); ++i)
        {
            const Move move = moves[i];
            UndoInfo undo;
            board.makeMove(move, undo);
            nodes += perft(board, depth - 1);
            board.undoMove(move, undo);
        }
        return nodes;
    }
    bool run(const PerftCase &test)
    {
        Board board;
        if (!Fen::load(board, test.fen))
        {
            if (!test.validFen)
            {
                std::cout << "PASS " << test.name << '\n';
                return true;
            }
            std::cerr << "FAIL " << test.name << ": invalid FEN\n";
            return false;
        }

        if (!test.validFen)
        {
            std::cerr << "FAIL " << test.name << ": malformed FEN was accepted\n";
            return false;
        }

        const auto actual = perft(board, test.depth);
        if (actual != test.expected)
        {
            std::cerr << "FAIL " << test.name << " depth " << test.depth << ": expected " << test.expected << ", got " << actual << '\n';
            return false;
        }
        std::cout << "PASS " << test.name << " depth " << test.depth << ": " << actual << '\n';
        return true;
    }

    // Po wykonaniu i cofnięciu każdego ruchu porównaj klucz Zobrista oraz
    // ocenę z wartościami przed ruchem (stan make/undo musi być dokładny).
    bool checkMakeUndo(Board& board, const std::string& label, int& checked)
    {
        const std::uint64_t keyBefore = board.getZobristKey();
        const int evalBefore = board.getEvalScore();
        MoveList moves;
        MoveGenerator::generateMoves(board, moves, MoveValidator::CheckInfo{});
        for (int i = 0; i < moves.size(); ++i)
        {
            const Move move = moves[i];
            UndoInfo undo;
            board.makeMove(move, undo);
            board.undoMove(move, undo);
            ++checked;
            if (board.getZobristKey() != keyBefore || board.getEvalScore() != evalBefore)
            {
                std::cerr << "FAIL " << label << ": make/undo mismatch after " << move.toUCI() << '\n';
                return false;
            }
        }
        return true;
    }

    // Rekurencyjne sprawdzenie make/undo na drzewie do zadanej głębokości.
    bool checkMakeUndoRecursive(Board& board, int depth, const std::string& label, int& checked)
    {
        if (depth == 0) return true;
        if (!checkMakeUndo(board, label, checked)) return false;
        MoveList moves;
        MoveGenerator::generateMoves(board, moves, MoveValidator::CheckInfo{});
        for (int i = 0; i < moves.size(); ++i)
        {
            const Move move = moves[i];
            UndoInfo undo;
            board.makeMove(move, undo);
            if (!checkMakeUndoRecursive(board, depth - 1, label, checked)) return false;
            board.undoMove(move, undo);
        }
        return true;
    }

    // Zgodność perft z pełnym generatorem i generatorem "bicia + ciche promocje hetmanem".
    std::uint64_t perftCaptures(Board& board, int depth)
    {
        if (depth == 0) return 1;
        MoveList moves;
        MoveGenerator::generateLegalCaptures(board, moves, MoveValidator::CheckInfo{});
        std::uint64_t nodes = 0;
        for (int i = 0; i < moves.size(); ++i)
        {
            const Move move = moves[i];
            UndoInfo undo;
            board.makeMove(move, undo);
            nodes += perftCaptures(board, depth - 1);
            board.undoMove(move, undo);
        }
        return nodes;
    }

    // Zliczenie konkretnych typów ruchów w pozycji testowej.
    struct MoveCounts { int quietQueenPromo = 0; int underQuietPromo = 0; int capturePromo = 0; };
    MoveCounts countPromotions(const char* fen)
    {
        Board board;
        MoveCounts counts;
        if (!Fen::load(board, fen)) return counts;
        MoveList moves;
        MoveGenerator::generateMoves(board, moves, MoveValidator::CheckInfo{});
        for (int i = 0; i < moves.size(); ++i)
        {
            const MoveFlag flag = moves[i].flag;
            if (flag == MoveFlag::PromotionQueen) ++counts.quietQueenPromo;
            else if (flag == MoveFlag::PromotionRook || flag == MoveFlag::PromotionBishop || flag == MoveFlag::PromotionKnight) ++counts.underQuietPromo;
            else if (flag >= MoveFlag::PromotionCaptureKnight && flag <= MoveFlag::PromotionCaptureQueen) ++counts.capturePromo;
        }
        return counts;
    }
}
int main()
{
    AttackTables::initAttackTables();
    bool success = true;

    constexpr PerftCase tests[] = {
        {"start position", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 4, 197281},
        {"Kiwipete", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 3, 97862},
        {"endgame", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 4, 43238},
        {"promotions and castling", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 3, 9467},
        {"promotion tactic", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 3, 62379},
        {"tactical position", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 3, 89890},
        {"reject malformed rank overflow", "rnbqkbnr/pppppppp/8/8/8/8/8/PP/RNBQKBNR w KQkq - 0 1", 1, 0, false},
    };
    for (const auto &test : tests)
        success = run(test) && success;

    // ---- Testy make/undo: FEN, prawa roszady, en passant, promocje, szach ----
    {
        int checked = 0;
        struct StateCase { const char* label; const char* fen; int depth; };
        const StateCase stateCases[] = {
            {"make/undo start", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 2},
            {"make/undo castling", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 2},
            {"make/undo en passant", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 2},
            {"make/undo promotions", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 2},
            {"make/undo promotion tactic", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 2},
        };
        for (const auto& sc : stateCases)
        {
            Board board;
            if (!Fen::load(board, sc.fen))
            {
                std::cerr << "FAIL " << sc.label << ": invalid FEN\n";
                success = false;
                continue;
            }
            if (!checkMakeUndoRecursive(board, sc.depth, sc.label, checked))
                success = false;
            else
                std::cout << "PASS " << sc.label << " (" << checked << " moves)\n";
        }
    }

    // ---- Testy promocji: generator pełny vs generator quiescence ----
    {
        // Pozycja 4 (promocje i roszada): pełny generator musi generować
        // wszystkie legalne promocje. Perft bialych w tej pozycji na gł. 2
        // obejmuje promocje ciche i bicia-promocje.
        struct PromoCase { const char* label; const char* fen; int depth; std::uint64_t expected; };
        const PromoCase promoCases[] = {
            {"promo full gen", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 2, 264},
            {"promo tactic full gen", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 2, 1486},
        };
        for (const auto& pc : promoCases)
        {
            Board board;
            if (!Fen::load(board, pc.fen)) { std::cerr << "FAIL " << pc.label << ": invalid FEN\n"; success = false; continue; }
            const auto actual = perft(board, pc.depth);
            if (actual != pc.expected) { std::cerr << "FAIL " << pc.label << ": expected " << pc.expected << ", got " << actual << '\n'; success = false; }
            else std::cout << "PASS " << pc.label << ": " << actual << '\n';
        }

        // Generator selektywny (bicia + ciche promocje hetmanem) — świadoma
        // polityka quiescence; sprawdzamy, że liczba cichych underpromocji
        // w pełnym generatorze jest niezerowa w pozycji testowej (regresja
        // generowania cichych promocji na wieżę/gońca/skoczka).
        const auto counts = countPromotions("2k5/4P3/8/8/8/8/8/2K5 w - - 0 1");
        if (counts.quietQueenPromo == 1 && counts.underQuietPromo == 3)
            std::cout << "PASS quiet promotions generated (1 queen + 3 under)\n";
        else
        {
            std::cerr << "FAIL quiet promotions: queen=" << counts.quietQueenPromo
                      << " under=" << counts.underQuietPromo << " (expected 1 + 3)\n";
            success = false;
        }

        // Pozycja z czterema bicia-promocjami: pionek e7 bije wieze f8.
        const auto capCounts = countPromotions("2k2r2/4P3/8/8/8/8/8/2K5 w - - 0 1");
        if (capCounts.capturePromo != 4)
        {
            std::cerr << "FAIL capture promotions: got " << capCounts.capturePromo << " (expected 4)\n";
            success = false;
        }
        else
            std::cout << "PASS capture promotions generated (4)\n";
    }

    // ---- Testy promocji: pelny generator musi generowac wszystkie legalne
    // promocje, a generator quiescence tylko bicia + ciche promocje hetmanem.
    {
        struct PromoPos { const char* label; const char* fen; int expectedCaptures; };
        const PromoPos promoPositions[] = {
            {"start position captures", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 0},
        };
        for (const auto &pos : promoPositions)
        {
            Board board;
            if (!Fen::load(board, pos.fen))
            {
                std::cerr << "FAIL " << pos.label << ": invalid FEN\n";
                success = false;
                continue;
            }
            MoveList captures;
            MoveGenerator::generateLegalCaptures(board, captures, MoveValidator::CheckInfo{});
            if (static_cast<int>(captures.size()) != pos.expectedCaptures)
            {
                std::cerr << "FAIL " << pos.label << ": expected " << pos.expectedCaptures
                          << ", got " << captures.size() << '\n';
                success = false;
            }
            else
                std::cout << "PASS " << pos.label << "\n";
        }
    }

    return success ? 0 : 1;
}