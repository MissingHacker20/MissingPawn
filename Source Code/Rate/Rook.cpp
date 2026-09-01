#include "Rate/Rook.h"

#include <algorithm>

#include "Foundation/Bitboard.h"
#include "Foundation/Board.h"

namespace
{
// Pionki na linii - prosto z bitboardów (file mask)
inline Bitboard fileMaskBB(int file)
{
    constexpr Bitboard fileA = 0x0101010101010101ULL;
    return fileA << file;
}

int countPawnsOnFile(const Bitboards& bitboards, ChessColor pawnColor, int file)
{
    return countBits(bitboards.pawns[Bitboards::indexOf(pawnColor)] & fileMaskBB(file));
}

// Czy wieża jest na tej samej linii co król przeciwnika (atak na króla)
bool rookOnKingFile(const Board& board, ChessColor color, int file)
{
    ChessColor enemyColor = (color == ChessColor::White) ? ChessColor::Black : ChessColor::White;
    Square enemyKing = board.getKingSquare(enemyColor);
    if (enemyKing == Square::None) return false;
    int kingFile = static_cast<int>(enemyKing) % 8;
    return file == kingFile;
}

// Sprawdza czy wieże są połączone (na tym samym rzędzie, bez figur między)
bool areRooksConnected(const Bitboards& bitboards, ChessColor color)
{
    const int idx = Bitboards::indexOf(color);
    Bitboard rooks = bitboards.rooks[idx];
    if (countBits(rooks) < 2) return false;

    // Znajdź pierwszą i drugą wieżę
    Square rook1 = popLeastSignificantBit(rooks);
    Square rook2 = popLeastSignificantBit(rooks);

    int r1 = static_cast<int>(rook1);
    int r2 = static_cast<int>(rook2);

    // Jeśli na tym samym rzędzie
    if (r1 / 8 == r2 / 8)
    {
        int minFile = std::min(r1 % 8, r2 % 8);
        int maxFile = std::max(r1 % 8, r2 % 8);
        int rank = r1 / 8;

        // Figury ściśle między wieżami (minFile+1 .. maxFile-1) tego samego
        // koloru blokują połączenie
        Bitboard inBetween = 0;
        for (int f = minFile + 1; f < maxFile; ++f)
            inBetween |= fileMaskBB(f);
        inBetween &= 0xFFULL << (rank * 8);

        return (bitboards.occupied[idx] & inBetween) == 0;
    }

    return false;
}
}

int RookEvaluation::evaluate(const Board& board, const Bitboards& bitboards, ChessColor color)
{
    constexpr int Material = 500;
    constexpr int OpenFileBonus = 25;
    constexpr int SemiOpenFileBonus = 15;
    constexpr int SeventhRankBonus = 25;
    constexpr int KingFileBonus = 10;
    constexpr int RooksConnectedBonus = 15;

    const int idx = Bitboards::indexOf(color);
    const int seventhRank = color == ChessColor::White ? 6 : 1;

    int score = 0;
    int rookCount = 0;

    Bitboard rooks = bitboards.rooks[idx];
    while (rooks)
    {
        const Square square = popLeastSignificantBit(rooks);
        const int index = static_cast<int>(square);
        const int file = index % 8;
        const int rank = index / 8;

        ++rookCount;

        score += Material;

        // Linie otwarte i półotwarte
        const int ownPawnsOnFile = countPawnsOnFile(bitboards, color, file);
        if (ownPawnsOnFile == 0)
        {
            // Brak własnych pionów na linii
            const ChessColor enemyColor = (color == ChessColor::White) ? ChessColor::Black : ChessColor::White;
            const int enemyPawnsOnFile = countPawnsOnFile(bitboards, enemyColor, file);
            if (enemyPawnsOnFile == 0)
                score += OpenFileBonus;       // otwarta linia
            else
                score += SemiOpenFileBonus;    // półotwarta linia
        }

        // Siódmy rząd
        if (rank == seventhRank)
        {
            score += SeventhRankBonus;
        }

        // Atak na króla przeciwnika
        if (rookOnKingFile(board, color, file))
        {
            score += KingFileBonus;
        }
    }

    // Połączone wieże
    if (rookCount >= 2 && areRooksConnected(bitboards, color))
    {
        score += RooksConnectedBonus;
    }

    return score;
}
