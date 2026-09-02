#include "Rate/King.h"

#include <algorithm>
#include <cmath>

#include "Foundation/Bitboard.h"
#include "Foundation/Bitboards.h"
#include "Foundation/Board.h"
#include "Move/AttackTables.h"

// Unia ataków strony `attacker` budowana wyłącznie z gotowych bitboardów.
// Równoważna pierwotnej wersji skanującej planszę.
Bitboard computeAttackBoard(const Bitboards& bitboards, ChessColor attacker)
{
    const int idx = Bitboards::indexOf(attacker);

    Bitboard attacked = bitboards.pawnAttacks[idx]
                      | bitboards.knightAttacks[idx]
                      | bitboards.bishopAttacks[idx]
                      | bitboards.rookAttacks[idx]
                      | bitboards.queenAttacks[idx];

    // Ataki króla (brak dedykowanego pola w Bitboards - bierzemy z AttackTables)
    Bitboard kings = bitboards.kings[idx];
    while (kings)
    {
        const Square s = popLeastSignificantBit(kings);
        attacked |= AttackTables::kingAttacks(s);
    }

    return attacked;
}


namespace
{
// Buduje bitboard wszystkich pól atakowanych przez stronę `attacker`.
// Jest dokładnie równoważny zbiorowi pól, dla których pierwotna funkcja
// isSquareUnderAttack zwracałaby true, ale liczy ataki tylko raz dla całej
// planszy zamiast osobno dla każdego pola (duże przyspieszenie).

int countKingZoneAttacks(const Bitboards& bitboards, ChessColor color)
{
    const int idx = Bitboards::indexOf(color);
    Bitboard kingBB = bitboards.kings[idx];
    if (kingBB == 0) return 0;

    const Square kingSq = popLeastSignificantBit(kingBB);

    ChessColor enemy = (color == ChessColor::White) ? ChessColor::Black : ChessColor::White;

    // Jednorazowo policz wszystkie ataki przeciwnika na planszy.
    const Bitboard attacked = computeAttackBoard(bitboards, enemy);

    int kf = static_cast<int>(kingSq) % 8;
    int kr = static_cast<int>(kingSq) / 8;
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
    const int enemyIdx = Bitboards::indexOf(enemy);
    const Bitboard zone = AttackTables::kingAttacks(kingSq) | (1ULL << static_cast<int>(kingSq));
    attacks += 2 * countBits(bitboards.pawnAttacks[enemyIdx] & zone);
    attacks += 3 * countBits(bitboards.knightAttacks[enemyIdx] & zone);
    attacks += 4 * countBits(bitboards.bishopAttacks[enemyIdx] & zone);
    attacks += 5 * countBits(bitboards.rookAttacks[enemyIdx] & zone);
    attacks += 6 * countBits(bitboards.queenAttacks[enemyIdx] & zone);
    return attacks;
}

int countOpenFilesNearKing(const Bitboards& bitboards, ChessColor color)
{
    const int idx = Bitboards::indexOf(color);
    Bitboard kingBB = bitboards.kings[idx];
    if (kingBB == 0) return 0;

    Square kingSq = popLeastSignificantBit(kingBB);

    int kf = static_cast<int>(kingSq) % 8;
    int openCount = 0;

    constexpr Bitboard fileA = 0x0101010101010101ULL;
    const Bitboard ownPawns = bitboards.pawns[idx];

    for (int f = kf - 1; f <= kf + 1; ++f)
    {
        if (f < 0 || f >= 8) continue;
        if ((ownPawns & (fileA << f)) == 0)
        {
            ++openCount;
        }
    }
    return openCount;
}

double gamePhase(const Bitboards& bitboards)
{
    constexpr int TotalPhase = 24;
    int phase = TotalPhase;

    for (int c = 0; c < 2; ++c)
    {
        phase -= countBits(bitboards.knights[c]) * 1;
        phase -= countBits(bitboards.bishops[c]) * 1;
        phase -= countBits(bitboards.rooks[c])   * 2;
        phase -= countBits(bitboards.queens[c])  * 4;
    }

    return std::max(0.0, phase / static_cast<double>(TotalPhase));
}
}

int KingEvaluation::evaluate(const Board& board, const Bitboards& bitboards, ChessColor color)
{
    const Square kingSquare = board.getKingSquare(color);
    if (kingSquare == Square::None) return 0;

    const int index = static_cast<int>(kingSquare);
    const int file = index % 8;
    const int rank = index / 8;

    const double phase = gamePhase(bitboards);
    const bool isEndgame = phase < 0.3;

    const int idx = Bitboards::indexOf(color);

    int score = 0;

    if (!isEndgame)
    {
        // MIDDLEGAME - King safety
        const Bitboard ownPawns = bitboards.pawns[idx];
        const int shieldRank = rank + ((color == ChessColor::White) ? 1 : -1);

        if (shieldRank >= 0 && shieldRank < 8)
        {
            for (int shieldFile = file - 1; shieldFile <= file + 1; ++shieldFile)
            {
                if (shieldFile >= 0 && shieldFile < 8)
                {
                    const Square sq = static_cast<Square>(shieldRank * 8 + shieldFile);
                    if (getBit(ownPawns, sq))
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
                    const Square sq = static_cast<Square>(shieldRank2 * 8 + shieldFile);
                    if (getBit(ownPawns, sq))
                    {
                        if (shieldFile == file) score += 8;
                        else score += 5;
                    }
                }
            }
        }

        int zoneAttacks = countKingZoneAttacks(bitboards, color);
        score -= zoneAttacks * 5;

        // Otwarta linia obok króla zwiększa możliwość wejścia ciężkich figur.
        score -= countOpenFilesNearKing(bitboards, color) * 18;

        int openFiles = countOpenFilesNearKing(bitboards, color);
        score -= openFiles * 10;

        Bitboard defenders = bitboards.knights[idx] | bitboards.bishops[idx];
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

        const Bitboard pawnsCopy = bitboards.pawns[idx];
        if (pawnsCopy != 0)
        {
            int minDist = 14;
            Bitboard pawns = pawnsCopy;
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
