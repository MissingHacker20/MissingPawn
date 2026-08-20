#include "Rate/King.h"

#include <algorithm>
#include <cmath>

#include "Foundation/Bitboard.h"
#include "Foundation/Board.h"
#include "Move/AttackTables.h"

Bitboard computeAttackBoard(const Board& board, ChessColor attacker)
{
    const Piece pawn   = (attacker == ChessColor::White) ? Piece::WhitePawn   : Piece::BlackPawn;
    const Piece knight = (attacker == ChessColor::White) ? Piece::WhiteKnight : Piece::BlackKnight;
    const Piece bishop = (attacker == ChessColor::White) ? Piece::WhiteBishop : Piece::BlackBishop;
    const Piece rook   = (attacker == ChessColor::White) ? Piece::WhiteRook   : Piece::BlackRook;
    const Piece queen  = (attacker == ChessColor::White) ? Piece::WhiteQueen  : Piece::BlackQueen;
    const Piece king   = (attacker == ChessColor::White) ? Piece::WhiteKing   : Piece::BlackKing;

    Bitboard attacked = 0;

    // Pionki
    Bitboard bb = board.getBitboard(pawn);
    while (bb)
    {
        const Square s = popLeastSignificantBit(bb);
        attacked |= (attacker == ChessColor::White)
            ? AttackTables::whitePawnAttacks(s)
            : AttackTables::blackPawnAttacks(s);
    }

    // Skoczkowie
    bb = board.getBitboard(knight);
    while (bb)
    {
        const Square s = popLeastSignificantBit(bb);
        attacked |= AttackTables::knightAttacks(s);
    }

    // Król
    bb = board.getBitboard(king);
    while (bb)
    {
        const Square s = popLeastSignificantBit(bb);
        attacked |= AttackTables::kingAttacks(s);
    }

    // Slajdery: promienie aż do pierwszej figury (łącznie z nią)
    constexpr int RookDirs[4][2]   = {{1,0},{-1,0},{0,1},{0,-1}};
    constexpr int BishopDirs[4][2] = {{1,1},{1,-1},{-1,1},{-1,-1}};

    // Wieże i hetmany (linie proste)
    bb = board.getBitboard(rook) | board.getBitboard(queen);
    while (bb)
    {
        const Square s = popLeastSignificantBit(bb);
        int f = static_cast<int>(s) % 8;
        int r = static_cast<int>(s) / 8;
        for (int d = 0; d < 4; ++d)
        {
            int cf = f + RookDirs[d][0];
            int cr = r + RookDirs[d][1];
            while (cf >= 0 && cf < 8 && cr >= 0 && cr < 8)
            {
                attacked |= (1ULL << (cr * 8 + cf));
                if (board.pieceAt(static_cast<Square>(cr * 8 + cf)) != Piece::None)
                    break;
                cf += RookDirs[d][0];
                cr += RookDirs[d][1];
            }
        }
    }

    // Gońce i hetmany (przekątne)
    bb = board.getBitboard(bishop) | board.getBitboard(queen);
    while (bb)
    {
        const Square s = popLeastSignificantBit(bb);
        int f = static_cast<int>(s) % 8;
        int r = static_cast<int>(s) / 8;
        for (int d = 0; d < 4; ++d)
        {
            int cf = f + BishopDirs[d][0];
            int cr = r + BishopDirs[d][1];
            while (cf >= 0 && cf < 8 && cr >= 0 && cr < 8)
            {
                attacked |= (1ULL << (cr * 8 + cf));
                if (board.pieceAt(static_cast<Square>(cr * 8 + cf)) != Piece::None)
                    break;
                cf += BishopDirs[d][0];
                cr += BishopDirs[d][1];
            }
        }
    }

    return attacked;
}


namespace
{
// Buduje bitboard wszystkich pól atakowanych przez stronę `attacker`.
// Jest dokładnie równoważny zbiorowi pól, dla których pierwotna funkcja
// isSquareUnderAttack zwracałaby true, ale liczy ataki tylko raz dla całej
// planszy zamiast osobno dla każdego pola (duże przyspieszenie).

int countKingZoneAttacks(const Board& board, ChessColor color)
{
    Square kingSq = board.getKingSquare(color);
    if (kingSq == Square::None) return 0;

    ChessColor enemy = (color == ChessColor::White) ? ChessColor::Black : ChessColor::White;

    // Jednorazowo policz wszystkie ataki przeciwnika na planszy.
    const Bitboard attacked = computeAttackBoard(board, enemy);

    int idx = static_cast<int>(kingSq);
    int kf = idx % 8;
    int kr = idx / 8;
    int attacks = 0;

    for (int df = -1; df <= 1; ++df)
    {
        for (int dr = -1; dr <= 1; ++dr)
        {
            int f = kf + df;
            int r = kr + dr;
            if (f >= 0 && f < 8 && r >= 0 && r < 8)
            {
                if (getBit(attacked, static_cast<Square>(r * 8 + f)))
                    ++attacks;
            }
        }
    }
    return attacks;
}

int countOpenFilesNearKing(const Board& board, ChessColor color)
{
    Square kingSq = board.getKingSquare(color);
    if (kingSq == Square::None) return 0;

    Piece ownPawn = (color == ChessColor::White) ? Piece::WhitePawn : Piece::BlackPawn;
    int kf = static_cast<int>(kingSq) % 8;
    int openCount = 0;

    for (int f = kf - 1; f <= kf + 1; ++f)
    {
        if (f < 0 || f >= 8) continue;
        bool hasPawn = false;
        for (int r = 0; r < 8; ++r)
        {
            if (board.pieceAt(static_cast<Square>(r * 8 + f)) == ownPawn)
            {
                hasPawn = true;
                break;
            }
        }
        if (!hasPawn) ++openCount;
    }
    return openCount;
}

double gamePhase(const Board& board)
{
    constexpr int TotalPhase = 24;
    int phase = TotalPhase;

    for (int i = static_cast<int>(Piece::WhiteKnight); i <= static_cast<int>(Piece::BlackQueen); ++i)
    {
        if (i == static_cast<int>(Piece::WhiteKing) || i == static_cast<int>(Piece::BlackKing))
            continue;

        Piece p = static_cast<Piece>(i);
        int count = countBits(board.getBitboard(p));
        int value = 0;
        switch (p)
        {
        case Piece::WhiteKnight: case Piece::BlackKnight:
        case Piece::WhiteBishop: case Piece::BlackBishop: value = 1; break;
        case Piece::WhiteRook:   case Piece::BlackRook:   value = 2; break;
        case Piece::WhiteQueen:  case Piece::BlackQueen:  value = 4; break;
        default: break;
        }
        phase -= count * value;
    }

    return std::max(0.0, phase / static_cast<double>(TotalPhase));
}
}

int KingEvaluation::evaluate(const Board& board, ChessColor color)
{
    const Square kingSquare = board.getKingSquare(color);
    if (kingSquare == Square::None) return 0;

    const int index = static_cast<int>(kingSquare);
    const int file = index % 8;
    const int rank = index / 8;

    const double phase = gamePhase(board);
    const bool isEndgame = phase < 0.3;

    int score = 0;

    if (!isEndgame)
    {
        // MIDDLEGAME - King safety
        const Piece ownPawn = (color == ChessColor::White) ? Piece::WhitePawn : Piece::BlackPawn;
        const int shieldRank = rank + ((color == ChessColor::White) ? 1 : -1);

        if (shieldRank >= 0 && shieldRank < 8)
        {
            for (int shieldFile = file - 1; shieldFile <= file + 1; ++shieldFile)
            {
                if (shieldFile >= 0 && shieldFile < 8)
                {
                    Square sq = static_cast<Square>(shieldRank * 8 + shieldFile);
                    if (board.pieceAt(sq) == ownPawn)
                    {
                        if (shieldFile == file) score += 20;
                        else score += 12;
                    }
                }
            }
        }

        const int shieldRank2 = rank + ((color == ChessColor::White) ? 2 : -2);
        if (shieldRank2 >= 0 && shieldRank2 < 8)
        {
            for (int shieldFile = file - 1; shieldFile <= file + 1; ++shieldFile)
            {
                if (shieldFile >= 0 && shieldFile < 8)
                {
                    Square sq = static_cast<Square>(shieldRank2 * 8 + shieldFile);
                    if (board.pieceAt(sq) == ownPawn)
                    {
                        if (shieldFile == file) score += 8;
                        else score += 5;
                    }
                }
            }
        }

        int zoneAttacks = countKingZoneAttacks(board, color);
        score -= zoneAttacks * 15;

        int openFiles = countOpenFilesNearKing(board, color);
        score -= openFiles * 10;

        Piece knight = (color == ChessColor::White) ? Piece::WhiteKnight : Piece::BlackKnight;
        Piece bishop = (color == ChessColor::White) ? Piece::WhiteBishop : Piece::BlackBishop;

        Bitboard defenders = board.getBitboard(knight) | board.getBitboard(bishop);
        Bitboard kingZone = 0;
        for (int df = -2; df <= 2; ++df)
        {
            for (int dr = -2; dr <= 2; ++dr)
            {
                int f = file + df;
                int r = rank + dr;
                if (f >= 0 && f < 8 && r >= 0 && r < 8)
                    setBit(kingZone, static_cast<Square>(r * 8 + f));
            }
        }
        if ((defenders & kingZone) == 0)
            score -= 15;
    }
    else
    {
        // ENDGAME - King activity
        const double centerDist = std::abs(file - 3.5) + std::abs(rank - 3.5);
        score += 30 - static_cast<int>(centerDist * 5);

        const Piece ownPawn = (color == ChessColor::White) ? Piece::WhitePawn : Piece::BlackPawn;
        Bitboard pawns = board.getBitboard(ownPawn);
        if (pawns != 0)
        {
            int minDist = 14;
            while (pawns)
            {
                Square psq = popLeastSignificantBit(pawns);
                int pidx = static_cast<int>(psq);
                int pf = pidx % 8, pr = pidx / 8;
                int dist = std::abs(file - pf) + std::abs(rank - pr);
                minDist = std::min(minDist, dist);
            }
            score += (8 - minDist) * 3;
        }
    }

    return score;
}
