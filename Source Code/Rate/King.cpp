#include "Rate/King.h"

#include <algorithm>
#include <cmath>

#include "Foundation/Bitboard.h"
#include "Foundation/Bitboards.h"
#include "Foundation/Board.h"
#include "Move/AttackTables.h"


namespace
{
int countKingZoneAttacks(const Bitboards& bitboards, ChessColor color)
{
    const int idx = Bitboards::indexOf(color);
    Bitboard kingBB = bitboards.kings[idx];
    if (kingBB == 0) return 0;

    const Square kingSq = popLeastSignificantBit(kingBB);

    ChessColor enemy = (color == ChessColor::White) ? ChessColor::Black : ChessColor::White;

    const int enemyIdx = Bitboards::indexOf(enemy);
    const Bitboard zone = AttackTables::kingAttacks(kingSq) | (1ULL << static_cast<int>(kingSq));

    // Each attacked square is counted once per attacker type.  The weights are
    // the complete definition of this feature; there is no extra base count.
    return 2 * countBits(bitboards.pawnAttacks[enemyIdx] & zone)
         + 3 * countBits(bitboards.knightAttacks[enemyIdx] & zone)
         + 4 * countBits(bitboards.bishopAttacks[enemyIdx] & zone)
         + 5 * countBits(bitboards.rookAttacks[enemyIdx] & zone)
         + 6 * countBits(bitboards.queenAttacks[enemyIdx] & zone);
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
                        if (shieldFile == file) score += 200;
                        else score += 120;
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
                        if (shieldFile == file) score += 80;
                        else score += 50;
                    }
                }
            }
        }

        int zoneAttacks = countKingZoneAttacks(bitboards, color);
        score -= zoneAttacks * 50;

        // Brak własnego pionka na pliku króla ułatwia wejście ciężkich figur.
        score -= countOpenFilesNearKing(bitboards, color) * 180;

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
            score -= 150;
    }
    else
    {
        // ENDGAME - King activity
        const double centerDist = std::abs(file - 3.5) + std::abs(rank - 3.5);
        score += 300 - static_cast<int>(centerDist * 50);

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
            score += (8 - minDist) * 30;
        }
    }

    return score;
}
