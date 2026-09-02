#include "Rate/Queen.h"

#include <cstdlib>
#include <algorithm>

#include "Move/AttackTables.h"
#include "Foundation/Bitboard.h"
#include "Foundation/Board.h"

namespace
{
int countQueenMobility(const Bitboards& bitboards, ChessColor color, Square from)
{
    Bitboard attacks = AttackTables::queenAttacks(from, bitboards.allOccupied);
    attacks &= ~bitboards.occupied[Bitboards::indexOf(color)];
    return countBits(attacks);
}

// Sprawdza czy hetman jest za wcześnie wyprowadzony (early queen development penalty)
bool isEarlyQueenDevelopment(const Bitboards& bitboards, ChessColor color, int rank)
{
    int devRank = (color == ChessColor::White) ? rank : 7 - rank;

    // Jeśli hetman opuścił swój początkowy rząd przed rozwinięciem innych figur
    if (devRank >= 2)
    {
        // Sprawdź czy skoczkowie i gońce są jeszcze na miejscu
        const int idx = Bitboards::indexOf(color);
        int knightsHome = countBits(bitboards.knights[idx]);
        int bishopsHome = countBits(bitboards.bishops[idx]);

        // Jeśli większość lekkich figur jeszcze nie rozwinięta
        return (knightsHome + bishopsHome >= 3);
    }
    return false;
}
}

int QueenEvaluation::evaluate(const Board& /*board*/, const Bitboards& bitboards, ChessColor color)
{
    constexpr int Material = 900;
    constexpr int MobBonusPerSquare = 2;
    constexpr int MobMax = 60;
    constexpr int VulnerableToMinorOrPawnPenalty = 200;
    constexpr int EarlyQueenPenalty = 30;

    const int idx = Bitboards::indexOf(color);
    int score = 0;

    Bitboard queens = bitboards.queens[idx];
    while (queens)
    {
        const Square square = popLeastSignificantBit(queens);
        const int index = static_cast<int>(square);
        const int file = index % 8;
        const int rank = index / 8;

        score += Material;

        // Centrum (delikatny bonus)
        const double centerDist = std::abs(file - 3.5) + std::abs(rank - 3.5);
        score += 12 - static_cast<int>(centerDist * 2);

        // Kara za wczesne wyprowadzenie
        if (isEarlyQueenDevelopment(bitboards, color, rank))
        {
            score -= EarlyQueenPenalty;
        }

        // Mobilność: -5 milipionów po konwersji całej oceny na MP.
        const int mobility = countQueenMobility(bitboards, color, square);
        score += std::max(0, std::min(mobility * MobBonusPerSquare, MobMax) - 5);

        const ChessColor enemy = color == ChessColor::White ? ChessColor::Black : ChessColor::White;
        const Bitboard enemyMinorPawn = bitboards.pawnAttacks[Bitboards::indexOf(enemy)] |
            bitboards.knightAttacks[Bitboards::indexOf(enemy)] |
            bitboards.bishopAttacks[Bitboards::indexOf(enemy)];
        if (getBit(enemyMinorPawn, square)) score -= VulnerableToMinorOrPawnPenalty / 10;
    }

    return score;
}
