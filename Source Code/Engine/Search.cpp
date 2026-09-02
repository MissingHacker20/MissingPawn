#include "Engine/Search.h"

#include <algorithm>
#include <limits>
#include <iostream>
#include <chrono>
#include <cstdint>

#include "Game/GameState.h"
#include "Game/GameHistory.h"
#include "Engine/MoveOrdering.h"
#include "Engine/KillerMoves.h"
#include "Engine/HistoryHeuristic.h"
#include "Move/MoveGenerator.h"
#include "Move/MoveValidator.h"
#include "Rate/Evaluation.h"
#include "Others/TimeManager.h"

namespace
{
constexpr int Infinity = std::numeric_limits<int>::max() / 2;

auto searchStartTime = std::chrono::steady_clock::time_point{};
int currentIterativeDepth = 0;

// Piece values for MVV-LVA scoring
constexpr int PieceValue[]
{
    0,
    1000,   // WhitePawn (milipawns)
    3200,   // WhiteKnight
    3300,   // WhiteBishop
    5000,   // WhiteRook
    9000,   // WhiteQueen
    200000, // WhiteKing
    1000,   // BlackPawn
    3200,   // BlackKnight
    3300,   // BlackBishop
    5000,   // BlackRook
    9000,   // BlackQueen
    200000  // BlackKing
};

inline int valueOfPiece(Piece piece)
{
    if (piece == Piece::None) return 0;
    return PieceValue[static_cast<int>(piece)];
}

// Lightweight MVV-LVA score for quiescence move sorting
inline int mvvLvaScore(const Move& move)
{
    if (move.capturedPiece == Piece::None) return 0;
    int victim = valueOfPiece(move.capturedPiece);
    int attacker = valueOfPiece(move.piece);
    return victim * 100 - attacker;
}

int nonPawnMaterialCount(const Board& board)
{
    return countBits(board.getBitboard(Piece::WhiteKnight)) +
           countBits(board.getBitboard(Piece::WhiteBishop)) +
           countBits(board.getBitboard(Piece::WhiteRook)) +
           countBits(board.getBitboard(Piece::WhiteQueen)) +
           countBits(board.getBitboard(Piece::BlackKnight)) +
           countBits(board.getBitboard(Piece::BlackBishop)) +
           countBits(board.getBitboard(Piece::BlackRook)) +
           countBits(board.getBitboard(Piece::BlackQueen));
}

bool safeForNullMove(const Board& board, ChessColor side)
{
    const Piece rook = side == ChessColor::White ? Piece::WhiteRook : Piece::BlackRook;
    const Piece queen = side == ChessColor::White ? Piece::WhiteQueen : Piece::BlackQueen;
    const bool hasMajor = board.getBitboard(rook) != 0 || board.getBitboard(queen) != 0;

    // Null move is unreliable in sparse minor/pawn endings: zugzwang is common
    // and passing is not a realistic legal option there.
    return hasMajor || nonPawnMaterialCount(board) >= 4;
}
}

// PV table definition
Move Search::pvTable[MaxPly][MaxPly];
int Search::pvLength[MaxPly];

// PV z ostatniej w pełni ukończonej iteracji IDDFS
Move Search::completedPvTable[MaxPly][MaxPly];
int Search::completedPvLength[MaxPly];

std::unordered_map<uint64_t, int> Search::repetitionCounts;

bool Search::ponderEnabled = false;

// RAII guard zapewniający zdjęcie pozycji ze ścieżki przy wyjściu
// z węzła (niezależnie od tego, którędy następuje return).
namespace
{
struct NodeRepGuard
{
    uint64_t key;
    explicit NodeRepGuard(uint64_t k) : key(k) {}
    ~NodeRepGuard() { Search::exitNode(key); }
};
}

bool Search::enterNode(uint64_t zobristKey)
{
    repetitionCounts[zobristKey]++;
    return repetitionCounts[zobristKey] >= 3;
}

void Search::exitNode(uint64_t zobristKey)
{
    auto it = repetitionCounts.find(zobristKey);
    if (it != repetitionCounts.end())
    {
        if (--(it->second) <= 0)
        {
            repetitionCounts.erase(it);
        }
    }
}

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
    if (!ponderEnabled && TimeManager::shouldStop())
    {
        return true;
    }

    return false;
}

Move Search::findBestMove(Board& board, int depth)
{
    searchStartTime = std::chrono::steady_clock::now();

    TimeManager::resetNodeCount();

    Move bestMove;
    Move completedBestMove;
    int completedDepth = 0;

    //--------------------------------------------------
    // Inicjalizacja śledzenia powtórzeń na ścieżce searchu.
    // Zaszczepiamy liczniki pozycjami z historii gry (z wyłączeniem
    // samej pozycji korzenia), aby powtórzenia z rzeczywistej partii
    // również były wykrywane wewnątrz symulowanych ruchów.
    //--------------------------------------------------
    repetitionCounts.clear();
    {
        const auto& history = GameHistory::getPositions();
        for (size_t i = 0; i + 1 < history.size(); ++i)
        {
            repetitionCounts[history[i]]++;
        }
    }

    //--------------------------------------------------
    // Szybka ścieżka: wymuszenie ruchu / mat w 1
    //--------------------------------------------------

    MoveList rootMoves;
    const MoveValidator::CheckInfo pseudoInfo{};
    MoveGenerator::generateMoves(board, rootMoves, pseudoInfo);
    MoveValidator::filterLegalMoves(board, rootMoves);

    if (rootMoves.size() == 0)
    {
        // Brak legalnych ruchów: mat lub pat (UCI wysyła 0000).
        std::cout << "bestmove 0000" << std::endl;
        return Move{};
    }

    if (rootMoves.size() == 1)
    {
        // Tylko jeden legalny ruch - zagraj go od razu.
        bestMove = rootMoves[0];
        std::cout << "bestmove " << bestMove.toUCI() << std::endl;
        return bestMove;
    }

    // Bez pełnej iteracji (np. przy bardzo krótkim movetime) zwracamy
    // przynajmniej legalny ruch, nigdy pusty ani częściowo oceniony wariant.
    bestMove = rootMoves[0];

    // Mat w 1: jeśli któryś ruch daje pozycję bez legalnych ruchów
    // przeciwnika w szachu, zagraj go natychmiast.
    for (int i = 0; i < rootMoves.size(); ++i)
    {
        const Move& move = rootMoves[i];
        UndoInfo undoInfo;
        board.makeMove(move, undoInfo);

        MoveList oppMoves;
        MoveGenerator::generateMoves(board, oppMoves, pseudoInfo);
        MoveValidator::filterLegalMoves(board, oppMoves);
        const bool oppInCheck =
            MoveValidator::isKingInCheck(board, board.getSideToMove());

        board.undoMove(move, undoInfo);

        if (oppMoves.size() == 0 && oppInCheck)
        {
            bestMove = move;
            std::cout << "bestmove " << bestMove.toUCI() << std::endl;
            return bestMove;
        }
    }

    //--------------------------------------------------
    // Iterative Deepening
    //--------------------------------------------------

    for (int currentDepth = 1; currentDepth <= depth; currentDepth++)
    {
        currentIterativeDepth = currentDepth;

        // Clear PV table for this depth
        for (int i = 0; i < MaxPly; i++)
        {
            pvLength[i] = 0;
        }

        MoveList moves;
        MoveGenerator::generateMoves(board, moves, pseudoInfo);
        MoveValidator::filterLegalMoves(board, moves);

        // Cache tactical evaluation for move ordering (hanging piece penalty)
        int tacticalEval = MoveValidator::evaluateTactics(board);

        // Posortuj root z poprzednim bestMove jako TT move (jeśli istnieje).
        // To utrzymuje stabilność, ale nie blokuje silnika - jeśli inny ruch
        // okaże się lepszy, zostanie wybrany (bo przeszukujemy wszystkie ruchy).
        MoveOrdering::sortMoves(board, moves, currentDepth, 0, bestMove, tacticalEval);

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
            TimeManager::incrementNodeCount();

            int score;

            if (index == 0)
            {
                // Pierwszy ruch (np. poprzedni bestMove) - pełne okno
                score = -negamax(board, currentDepth - 1, -beta, -alpha, 1);
            }
            else
            {
                // Zwężone okno (PVS) - szybkie odrzucenie słabszych ruchów
                score = -negamax(board, currentDepth - 1, -alpha - 1, -alpha, 1);

                // Jeśli wynik mieści się w oknie, przeszukaj pełne okno (re-search)
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

                // Store PV at root: bestMove + PV from child
                pvTable[0][0] = bestMove;
                int childPly = 1;
                for (int i = 0; i < pvLength[1]; i++)
                {
                    pvTable[0][childPly] = pvTable[1][i];
                    childPly++;
                }
                pvLength[0] = childPly;
            }

            alpha = std::max(alpha, score);
        }

        // Jeśli czas minął, zakończ iteracyjne pogłębianie
        if (shouldStopSearch())
        {
            break;
        }

        //--------------------------------------------------
        // Iteracja ukończona w pełni - skopiuj PV do completedPvTable
        //--------------------------------------------------
        for (int i = 0; i < MaxPly; i++)
        {
            completedPvLength[i] = pvLength[i];
            for (int j = 0; j < pvLength[i]; j++)
            {
                completedPvTable[i][j] = pvTable[i][j];
            }
        }

        completedBestMove = bestMove;
        completedDepth = currentDepth;

        //--------------------------------------------------
        // UCI info output using completed PV table
        //--------------------------------------------------

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - searchStartTime).count();

        uint64_t nodes = TimeManager::getNodesSearched();
        uint64_t nps = (elapsed > 0)
            ? nodes * 1000 / static_cast<uint64_t>(elapsed)
            : 0;

        std::cout << "info"
                  << " depth " << currentDepth
                  << " score cp " << bestScore / 10
                  << " nodes " << nodes
                  << " nps " << nps
                  << " time " << elapsed
                  << " pv";

        // Print PV: current iteration + completed iteration to fill up to depth
        int pvPrinted = 0;
        for (int i = 0; i < pvLength[0]; i++)
        {
            if (pvTable[0][i].from == Square::None)
                break;
            std::cout << " " << pvTable[0][i].toUCI();
            pvPrinted++;
        }
        // Fill remaining from completed PV (full iteration)
        for (int i = pvPrinted; i < completedPvLength[0] && pvPrinted < currentDepth; i++)
        {
            if (completedPvTable[0][i].from == Square::None)
                break;
            std::cout << " " << completedPvTable[0][i].toUCI();
            pvPrinted++;
        }

        std::cout << std::endl;
    }

    // Wynik przerwanej iteracji nie jest wiarygodny. Zachowujemy wyłącznie
    // ruch z ostatniej w całości przeanalizowanej głębokości.
    if (completedBestMove.from != Square::None)
    {
        bestMove = completedBestMove;
        currentIterativeDepth = completedDepth;
    }

    // Store in history (używamy głębokości ostatniej ukończonej iteracji)
    if (bestMove.from != Square::None)
    {
        HistoryHeuristic::add(
            board.getSideToMove(),
            bestMove,
            currentIterativeDepth);
    }

    // UCI: gdy brak legalnego ruchu, wyślij "0000" zamiast pustego ruchu
    if (bestMove.from == Square::None)
    {
        std::cout << "bestmove 0000" << std::endl;
    }
    else
    {
        std::cout << "bestmove " << bestMove.toUCI() << std::endl;
    }

    return bestMove;
}

int Search::quiesce(Board& board, int alpha, int beta, int ply)
{
    TimeManager::incrementNodeCount();

    if (shouldStopSearch())
    {
        return 0;
    }

    const bool inCheck = MoveValidator::isKingInCheck(board, board.getSideToMove());
    const MoveValidator::CheckInfo pseudoInfo{};

    MoveList moves;
    if (inCheck)
    {
        MoveGenerator::generateMoves(board, moves, pseudoInfo);
    }
    else
    {
        MoveGenerator::generateCaptures(board, moves, pseudoInfo);
    }
    MoveValidator::filterLegalMoves(board, moves);
    MoveValidator::updatePieceBitboards(board, moves);

    // Cache tactical evaluation for delta/SEE pruning and move ordering reuse
    int tacticalEval = 0;
    if (!inCheck) {
        tacticalEval = MoveValidator::evaluateTactics(board);
    }

    // Use full move ordering with tactical evaluation for hanging piece penalty
    MoveOrdering::sortMoves(board, moves, 0, ply, Move(), tacticalEval);

    // W szachu nie można zastosować stand-pat: trzeba rozpatrzyć
    // wszystkie legalne odpowiedzi na szacha. Jeśli nie ma ruchów,
    // to jest mat (zwracamy dużą ujemną wartość z perspektywy
    // strony do ruchu).
    int standPat = 0;

    if (inCheck)
    {
        if (moves.size() == 0)
        {
            return -MateScore + ply;
        }
    }
    else
    {
        standPat = Evaluation::evaluate(board);
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
    }

    for (int i = 0; i < moves.size(); i++)
    {
        if (shouldStopSearch())
        {
            return alpha;
        }

        const Move& move = moves[i];

        // Delta pruning: depth-dependent margins, protect rooks/queens
        int pieceValue = 0;
        switch (move.capturedPiece)
        {
        case Piece::WhitePawn:   case Piece::BlackPawn:   pieceValue = 1000;  break;
        case Piece::WhiteKnight: case Piece::BlackKnight: pieceValue = 3200;  break;
        case Piece::WhiteBishop: case Piece::BlackBishop: pieceValue = 3300;  break;
        case Piece::WhiteRook:   case Piece::BlackRook:   pieceValue = 5000;  break;
        case Piece::WhiteQueen:  case Piece::BlackQueen:  pieceValue = 9000;  break;
        default: pieceValue = 0; break;
        }

        // Delta margins are expressed in milipawns.
        int deltaMargin = 2000 - std::min(ply, 12) * 120;
        deltaMargin = std::max(deltaMargin, 500);

        // Don't prune rook/queen captures (value >= 5000)
        // For minor pieces, use depth-dependent margin
        if (!inCheck && pieceValue < 5000 &&
            standPat + pieceValue + deltaMargin < alpha)
        {
            continue;
        }

        // SEE must describe the exchange in the parent position.  Keep it
        // before makeMove; after the move the victim is no longer on `to`.
        const int seeScore = (!inCheck && move.capturedPiece != Piece::None)
            ? MoveValidator::see(board, move.to) : 0;

        // SEE pruning: -1000 MP still allows protected tactical follow-ups.
        if (!inCheck && move.capturedPiece != Piece::None && seeScore < -1000)
        {
            continue;
        }

        UndoInfo undoInfo;
        board.makeMove(move, undoInfo);

        // Selective recapture extension: only for high-value captures (rook/queen)
        // or when SEE is unclear (near zero, meaning tactical complexity)
        bool recaptureExtension = false;
        if (move.capturedPiece != Piece::None)
        {
            int victimValue = valueOfPiece(move.capturedPiece);

            // Extend if: rook/queen capture, or SEE within +/-2000 MP.
            if (victimValue >= 5000 || (seeScore > -2000 && seeScore < 2000))
            {
                MoveList oppMoves;
                MoveGenerator::generateCaptures(board, oppMoves, pseudoInfo);
                MoveValidator::filterLegalMoves(board, oppMoves);
                for (int j = 0; j < oppMoves.size(); ++j)
                {
                    if (oppMoves[j].to == move.to && oppMoves[j].capturedPiece != Piece::None)
                    {
                        recaptureExtension = true;
                        break;
                    }
                }
            }
        }

        int searchDepth = ply + 1 + (recaptureExtension ? 1 : 0);
        const int score = -quiesce(board, -beta, -alpha, searchDepth);

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
    TimeManager::incrementNodeCount();

    // Initialize PV for this node
    pvLength[ply] = 0;

    if (shouldStopSearch())
    {
        return 0;
    }

    const uint64_t nodeKey = board.getZobristKey();

    // Powtórzenie na ścieżce searchu (lub w historii gry) - remis.
    if (enterNode(nodeKey))
    {
        exitNode(nodeKey);
        return 0;
    }
    NodeRepGuard repGuard(nodeKey);

    //--------------------------------------------------
    // Generate moves once (reused for terminal detection and search)
    //--------------------------------------------------

    const MoveValidator::CheckInfo pseudoInfo{};

    MoveList moves;
    MoveGenerator::generateMoves(board, moves, pseudoInfo);
    MoveValidator::filterLegalMoves(board, moves);
    MoveValidator::updatePieceBitboards(board, moves);

    // Cache tactical evaluation for move ordering (hanging piece penalty)
    int tacticalEval = MoveValidator::evaluateTactics(board);

    const int endScore = terminalScore(board, moves, ply);
    if (endScore != 0)
    {
        return endScore;
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

    if (depth >= 3 && ply > 0 &&
        !MoveValidator::isKingInCheck(board, board.getSideToMove()) &&
        safeForNullMove(board, board.getSideToMove()))
    {
        UndoInfo nullUndo;
        board.makeNullMove(nullUndo);

        int R = (depth > 6) ? 3 : 2;
        int nullScore = -negamax(board, depth - 1 - R, -beta, -beta + 1, ply + 1);

        board.undoNullMove(nullUndo);

        if (nullScore >= beta)
        {
            return beta;
        }
    }

    MoveOrdering::sortMoves(board, moves, depth, ply, Move(), tacticalEval);

    Move bestMove;
    int bestScore = -Infinity;

    for (int index = 0; index < moves.size(); ++index)
    {
        // Jeśli czas minął, przerwij natychmiast — nie przeszukuj już
        // pozostałych ruchów na tym poziomie (szybkie zatrzymanie).
        if (shouldStopSearch())
        {
            break;
        }

        const Move& move = moves[index];

        // LMR: redukujemy tylko ruchy ciche o niskim prawdopodobieństwie
        // istotności. Szach, killer, historia i ruch wysoko ustawiony przez
        // ordering zachowują pełną głębokość.
        bool doReduction = false;
        int reduction = 0;
        const int historyScore = HistoryHeuristic::get(board.getSideToMove(), move);
        const bool isKiller = KillerMoves::score(ply, move) > 0;
        const bool highQuality = index < 6 || historyScore >= 100 || isKiller;

        if (index >= 3 && depth >= 3 && !highQuality &&
            move.capturedPiece == Piece::None &&
            move.flag != MoveFlag::KingCastle &&
            move.flag != MoveFlag::QueenCastle &&
            !(move.flag >= MoveFlag::PromotionKnight &&
              move.flag <= MoveFlag::PromotionCaptureQueen))
        {
            doReduction = true;
            reduction = (index >= 8 && depth >= 5) ? 2 : 1;
        }

        UndoInfo undoInfo;
        board.makeMove(move, undoInfo);

        // LMR is based on the move and child position, not on the net tactical
        // score of the parent (which can cancel to zero).
        const bool givesCheck = MoveValidator::isKingInCheck(board, board.getSideToMove());
        if (givesCheck)
        {
            doReduction = false;
        }

        int score;

        if (index == 0)
        {
            // Pierwszy ruch (PV move) - pełne okno search
            score = -negamax(board, depth - 1, -beta, -alpha, ply + 1);
        }
        else
        {
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

            // Re-search z pełnym oknem tylko dla kolejnych ruchów (index > 0) gdy poprawiły alpha
            if (score > alpha && score < beta)
            {
                score = -negamax(board, depth - 1, -beta, -alpha, ply + 1);
            }
        }

        board.undoMove(move, undoInfo);

        // Verification search: if we captured a high-value piece (rook/queen)
        // but the score failed low, re-search with +1 depth to verify
        // there isn't a tactical refutation we missed at reduced depth.
        // Skip when time is critical to avoid time trouble.
        if (move.capturedPiece != Piece::None && score <= alpha && !TimeManager::shouldStop())
        {
            int victimValue = 0;
            switch (move.capturedPiece)
            {
            case Piece::WhiteRook:   case Piece::BlackRook:   victimValue = 5000; break;
            case Piece::WhiteQueen:  case Piece::BlackQueen:  victimValue = 9000; break;
            default: victimValue = 0; break;
            }

            // Only verify at sufficient depth (>=4) and for rook/queen
            if (victimValue >= 5000 && depth >= 4)
            {
                UndoInfo verifyUndo;
                board.makeMove(move, verifyUndo);
                int verifyScore = -negamax(board, depth, -beta, -alpha, ply + 1);
                board.undoMove(move, verifyUndo);
                score = verifyScore;
            }
        }

        if (score > bestScore)
        {
            bestScore = score;
            bestMove = move;

            // Update PV: this move + PV from child
            if (ply < MaxPly - 1)
            {
                pvTable[ply][0] = move;
                int childPly = ply + 1;
                for (int i = 0; i < pvLength[childPly]; i++)
                {
                    pvTable[ply][1 + i] = pvTable[childPly][i];
                }
                pvLength[ply] = 1 + pvLength[childPly];
            }
        }

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
                KillerMoves::add(ply, move);
                HistoryHeuristic::add(board.getSideToMove(), move, depth);
            }

            break;
        }
    }

    return bestScore;
}

int Search::terminalScore(
    const Board& board,
    const MoveList& moves,
    int ply)
{
    // Brak legalnych ruchów (lista jest pseudo-legalna, więc jej
    // pustka oznacza brak ruchów w ogóle).
    if (moves.size() == 0)
    {
        const bool inCheck =
            MoveValidator::isKingInCheck(board, board.getSideToMove());

        if (inCheck)
        {
            // Mat: strona do ruchu jest zamatowana.
            return -MateScore + ply;
        }

        // Pat.
        return 0;
    }

    // Zasada 50 ruchów.
    if (board.getHalfmoveClock() >= 100)
    {
        return 0;
    }

    return 0;
}
