#include "Rate/Rook.h"

#include <algorithm>

#include "Foundation/Board.h"

namespace
{
int countPawnsOnFile(const Board& board, Piece pawn, int file)
{
    int count = 0;
    for (int rank = 0; rank < 8; ++rank)
    {
        if (board.pieceAt(static_cast<Square>(rank * 8 + file)) == pawn)
        {
            ++count;
        }
    }
    return count;
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
bool areRooksConnected(const Board& board, ChessColor color)
{
    Piece rook = (color == ChessColor::White) ? Piece::WhiteRook : Piece::BlackRook;
    Bitboard rooks = board.getBitboard(rook);
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

        for (int f = minFile + 1; f < maxFile; ++f)
        {
            Piece p = board.pieceAt(static_cast<Square>(rank * 8 + f));
            if (p != Piece::None &&
                getPieceColor(p) == color)
            {
                return false;
            }
        }
        return true;
    }

    return false;
}
}

int RookEvaluation::evaluate(const Board& board, ChessColor color)
{
    constexpr int Material = 500;
    constexpr int OpenFileBonus = 25;
    constexpr int SemiOpenFileBonus = 15;
    constexpr int SeventhRankBonus = 25;
    constexpr int KingFileBonus = 10;
    constexpr int RooksConnectedBonus = 15;

    const Piece rook = color == ChessColor::White ? Piece::WhiteRook : Piece::BlackRook;
    const Piece ownPawn = color == ChessColor::White ? Piece::WhitePawn : Piece::BlackPawn;
    const int seventhRank = color == ChessColor::White ? 6 : 1;

    int score = 0;
    int rookCount = 0;

    for (int index = 0; index < 64; ++index)
    {
        if (board.pieceAt(static_cast<Square>(index)) != rook)
        {
            continue;
        }

        ++rookCount;
        const int file = index % 8;
        const int rank = index / 8;

        score += Material;

        // Linie otwarte i półotwarte
        const int ownPawnsOnFile = countPawnsOnFile(board, ownPawn, file);
        if (ownPawnsOnFile == 0)
        {
            // Brak własnych pionów na linii
            const Piece enemyPawn = (color == ChessColor::White) ? Piece::BlackPawn : Piece::WhitePawn;
            const int enemyPawnsOnFile = countPawnsOnFile(board, enemyPawn, file);
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
    if (rookCount >= 2 && areRooksConnected(board, color))
    {
        score += RooksConnectedBonus;
    }

    return score;
}
