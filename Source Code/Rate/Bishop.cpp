#include "Rate/Bishop.h"
#include "Rate/Evaluation.h"

#include <cstdlib>
#include <algorithm>

#include "Move/AttackTables.h"
#include "Foundation/Bitboard.h"
#include "Foundation/Board.h"

namespace
{
int countBishopMobility(const Bitboards& bitboards, ChessColor color, Square from)
{
    Bitboard attacks = AttackTables::bishopAttacks(from, bitboards.allOccupied);
    attacks &= ~bitboards.occupied[Bitboards::indexOf(color)];
    return countBits(attacks);
}

// Sprawdza czy goniec jest "fianchetto" - na b7/g7/b2/g2 z własnym pionem przed
bool isFianchetto(const Bitboards& bitboards, ChessColor color, int file, int rank)
{
    if (!(file == 1 || file == 6)) return false; // b lub g

    const int expectedRank = (color == ChessColor::White) ? 1 : 6;
    if (rank != expectedRank) return false;

    // Sprawdź czy pion przed gońcem
    const int pawnRank = rank + ((color == ChessColor::White) ? 1 : -1);
    if (pawnRank >= 0 && pawnRank < 8)
    {
        return getBit(bitboards.pawns[Bitboards::indexOf(color)],
                      static_cast<Square>(pawnRank * 8 + file));
    }
    return false;
}

// Sprawdza czy goniec jest "wielkim" (długi diagonale)
bool isLongDiagonalBishop(int file, int rank)
{
    return (file == rank) || (file + rank == 7);
}

bool bishopQueenBattery(const Bitboards& b, ChessColor color, Square bishop)
{
    const int idx = Bitboards::indexOf(color);
    Bitboard queens = b.queens[idx];
    while (queens)
    {
        const Square queen = popLeastSignificantBit(queens);
        const int bf = static_cast<int>(bishop) % 8, br = static_cast<int>(bishop) / 8;
        const int qf = static_cast<int>(queen) % 8, qr = static_cast<int>(queen) / 8;
        if (std::abs(bf - qf) == std::abs(br - qr))
        {
            const int sf = qf > bf ? 1 : -1, sr = qr > br ? 1 : -1;
            bool clear = true;
            for (int f = bf + sf, r = br + sr; f != qf; f += sf, r += sr)
                if (getBit(b.allOccupied, static_cast<Square>(r * 8 + f))) clear = false;
            if (clear) return true;
        }
    }
    return false;
}
}

int BishopEvaluation::evaluate(const Board& /*board*/, const Bitboards& bitboards, ChessColor color)
{
    constexpr int Material = 3300;
    constexpr int BishopPairBonusMG = 280;
    constexpr int BishopPairBonusEG = 180;
    constexpr int FianchettoBonus = 80;
    constexpr int LongDiagBonus = 40;
    constexpr int CenterBonusMG = 120;
    constexpr int CenterBonusEG = 80;
    // EG mobility ~70% of MG
    static constexpr int mobilityMG[14] = {-20,-10,0,10,20,35,55,80,105,130,155,180,205,230};
    static constexpr int mobilityEG[14] = {-14,-7,0,7,14,25,39,56,74,91,109,126,144,161};

    const int idx = Bitboards::indexOf(color);
    int score = 0;
    int count = 0;

    Bitboard bishops = bitboards.bishops[idx];
    while (bishops)
    {
        const Square square = popLeastSignificantBit(bishops);

        ++count;
        const int index = static_cast<int>(square);
        const int file = index % 8;
        const int rank = index / 8;

        score += Material;

        // Aktywność zależy od faktycznej mobilności, a nie samej geometrii.
        const int mobility = countBishopMobility(bitboards, color, square);
        const int phase = Evaluation::gamePhase(bitboards);
        const int m = std::min(mobility, 13);
        score += (mobilityMG[m] * phase + mobilityEG[m] * (24 - phase)) / 24;

        // Fianchetto
        if (isFianchetto(bitboards, color, file, rank))
        {
            score += FianchettoBonus;
        }

        // Długa diagonala
        if (isLongDiagonalBishop(file, rank))
        {
            score += LongDiagBonus;
        }

        // Centrum - delikatny bonus za bliskość centrum
        const double centerDist = std::abs(file - 3.5) + std::abs(rank - 3.5);
        const int centerMG = CenterBonusMG - static_cast<int>(centerDist * 20);
        const int centerEG = CenterBonusEG - static_cast<int>(centerDist * 15);
        if (centerMG > 0) score += (centerMG * phase + centerEG * (24 - phase)) / 24;

        // Fianchetto i długa przekątna są tylko potencjałem; mobility powyżej
        // pokazuje, czy goniec rzeczywiście jest aktywny.
        if (bishopQueenBattery(bitboards, color, square) && mobility >= 5) score += 60;
    }

    // Para gońców
    if (count >= 2)
    {
        // Para jest silniejsza w otwartej pozycji.
        const int openSquares = 64 - countBits(bitboards.allOccupied);
        const int pairMG = BishopPairBonusMG + std::min(120, openSquares * 2);
        const int pairEG = BishopPairBonusEG + std::min(160, openSquares * 3);
        const int phase = Evaluation::gamePhase(bitboards);
        score += (pairMG * phase + pairEG * (24 - phase)) / 24;
    }

    return score;
}
