/*
 * MissingPawn
 * Copyright (C) 2026 Kacper Wieczorek
 *
 * This file is part of MissingPawn.
 *
 * Licensed under the GNU General Public License v3 or later.
 * See LICENSE for details.
 */

#include "Rate/Rook.h"
#include "Rate/Evaluation.h"

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

bool rookQueenBattery(const Bitboards& b, ChessColor color, Square rook)
{
    const int idx = Bitboards::indexOf(color);
    Bitboard queens = b.queens[idx];
    while (queens)
    {
        const Square queen = popLeastSignificantBit(queens);
        const int rf = static_cast<int>(rook) % 8, rr = static_cast<int>(rook) / 8;
        const int qf = static_cast<int>(queen) % 8, qr = static_cast<int>(queen) / 8;
        if (rf == qf || rr == qr)
        {
            const Bitboard ray = (rf == qf) ? fileMaskBB(rf) : (0xFFULL << (rr * 8));
            if ((b.allOccupied & ray & ~(1ULL << static_cast<int>(rook)) & ~(1ULL << static_cast<int>(queen))) == 0)
                return true;
        }
    }
    return false;
}

// Czy wieża jest na tej samej linii co król przeciwnika (atak na króla)
bool rookBehindPassedPawn(const Bitboards& b, ChessColor color, int rookFile, int rookRank)
{
    const int idx = Bitboards::indexOf(color);
    const int enemy = 1 - idx;
    const Bitboard own = b.pawns[idx];
    const Bitboard theirs = b.pawns[enemy];
    for (int rank = color == ChessColor::White ? rookRank + 1 : rookRank - 1;
         rank >= 0 && rank < 8; rank += color == ChessColor::White ? 1 : -1)
    {
        const Square pawnSq = static_cast<Square>(rank * 8 + rookFile);
        if (!getBit(own, pawnSq)) continue;
        bool passed = true;
        for (int f = std::max(0, rookFile - 1); f <= std::min(7, rookFile + 1); ++f)
        {
            for (int r = color == ChessColor::White ? rank + 1 : rank - 1;
                 r >= 0 && r < 8; r += color == ChessColor::White ? 1 : -1)
                if (getBit(theirs, static_cast<Square>(r * 8 + f))) passed = false;
        }
        if (passed) return true;
    }
    return false;
}

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

// Sprawdza czy wieża odcina króla przeciwnika (cut-off)
int rookCutOffLevel(const Bitboards& bitboards, ChessColor color, int rookFile, int rookRank)
{
    const int enemyIdx = 1 - Bitboards::indexOf(color);
    Bitboard enemyKingBB = bitboards.kings[enemyIdx];
    if (enemyKingBB == 0) return 0;

    Square enemyKing = popLeastSignificantBit(enemyKingBB);
    int kingFile = static_cast<int>(enemyKing) % 8;
    int kingRank = static_cast<int>(enemyKing) / 8;

    // Sprawdź czy wieża kontroluje linię lub rząd, ograniczając króla
    int ranksCutOff = 0;
    int filesCutOff = 0;

    // Wieża na tym samym rzędzie co król - odcina w pionie
    if (rookRank == kingRank)
    {
        // Sprawdź czy są pionki/figury między wieżą a królem
        int minF = std::min(rookFile, kingFile);
        int maxF = std::max(rookFile, kingFile);
        bool blocked = false;
        for (int f = minF + 1; f < maxF; ++f)
        {
            if (getBit(bitboards.allOccupied, static_cast<Square>(rookRank * 8 + f)))
            {
                blocked = true;
                break;
            }
        }
        if (!blocked)
        {
            // Król jest ograniczony do jednej strony wieży
            ranksCutOff = std::max(rookFile, 7 - rookFile);
        }
    }

    // Wieża na tym samym pliku co król - odcina w poziomie
    if (rookFile == kingFile)
    {
        int minR = std::min(rookRank, kingRank);
        int maxR = std::max(rookRank, kingRank);
        bool blocked = false;
        for (int r = minR + 1; r < maxR; ++r)
        {
            if (getBit(bitboards.allOccupied, static_cast<Square>(r * 8 + rookFile)))
            {
                blocked = true;
                break;
            }
        }
        if (!blocked)
        {
            filesCutOff = std::max(rookRank, 7 - rookRank);
        }
    }

    int totalCutOff = std::max(ranksCutOff, filesCutOff);
    if (totalCutOff >= 3) return 3;
    if (totalCutOff >= 2) return 2;
    return 0;
}
}

static std::pair<int, int> calculateRookScorePair(const Board& board, const Bitboards& bitboards, ChessColor color)
{
    constexpr int MaterialMG = 5192, MaterialEG = 5194;
    constexpr int OpenFileBonusMG = 273, OpenFileBonusEG = 275;
    constexpr int SemiOpenFileBonusMG = 185, SemiOpenFileBonusEG = 131;
    constexpr int SeventhRankBonusMG = 271, SeventhRankBonusEG = 281;
    constexpr int KingFileBonusMG = 119, KingFileBonusEG = 134;
    constexpr int RooksConnectedBonusMG = 180, RooksConnectedBonusEG = 180;
    constexpr int RookQueenBatteryBonusMG = 179, RookQueenBatteryBonusEG = 178;
    constexpr int PassedPawnSupportBonusMG = 145, PassedPawnSupportBonusEG = 333;
    constexpr int VulnerableToMinorOrPawnPenaltyMG = 8, VulnerableToMinorOrPawnPenaltyEG = 6;
    constexpr int RookCutoffMediumBonusMG = 150, RookCutoffMediumBonusEG = 150;
    constexpr int RookCutoffStrongBonusMG = 281, RookCutoffStrongBonusEG = 293;

    const int idx = Bitboards::indexOf(color);
    const int seventhRank = color == ChessColor::White ? 6 : 1;

    int mgScore = 0;
    int egScore = 0;
    int rookCount = 0;

    Bitboard rooks = bitboards.rooks[idx];
    while (rooks)
    {
        const Square square = popLeastSignificantBit(rooks);
        const int index = static_cast<int>(square);
        const int file = index % 8;
        const int rank = index / 8;

        ++rookCount;

        mgScore += MaterialMG;
        egScore += MaterialEG;

        // Linie otwarte i półotwarte
        const int ownPawnsOnFile = countPawnsOnFile(bitboards, color, file);
        if (ownPawnsOnFile == 0)
        {
            // Brak własnych pionów na linii
            const ChessColor enemyColor = (color == ChessColor::White) ? ChessColor::Black : ChessColor::White;
            const int enemyPawnsOnFile = countPawnsOnFile(bitboards, enemyColor, file);
            if (enemyPawnsOnFile == 0)
            {
                mgScore += OpenFileBonusMG;
                egScore += OpenFileBonusEG;
            }
            else
            {
                mgScore += SemiOpenFileBonusMG;
                egScore += SemiOpenFileBonusEG;
            }
        }

        // Siódmy rząd
        if (rank == seventhRank)
        {
            mgScore += SeventhRankBonusMG;
            egScore += SeventhRankBonusEG;
        }

        // Atak na króla przeciwnika
        if (rookOnKingFile(board, color, file))
        {
            mgScore += KingFileBonusMG;
            egScore += KingFileBonusEG;
        }
        if (rookBehindPassedPawn(bitboards, color, file, rank))
        {
            mgScore += PassedPawnSupportBonusMG;
            egScore += PassedPawnSupportBonusEG;
        }

        // Rook cut-off valuation is tapered between explicit MG and EG values.
        const int cutOffLevel = rookCutOffLevel(bitboards, color, file, rank);
        if (cutOffLevel >= 3)
        {
            mgScore += RookCutoffStrongBonusMG;
            egScore += RookCutoffStrongBonusEG;
        }
        else if (cutOffLevel >= 2)
        {
            mgScore += RookCutoffMediumBonusMG;
            egScore += RookCutoffMediumBonusEG;
        }

        const ChessColor enemy = color == ChessColor::White ? ChessColor::Black : ChessColor::White;
        const Bitboard enemyMinorPawn = bitboards.pawnAttacks[Bitboards::indexOf(enemy)] |
            bitboards.knightAttacks[Bitboards::indexOf(enemy)] |
            bitboards.bishopAttacks[Bitboards::indexOf(enemy)];
        if (getBit(enemyMinorPawn, square))
        {
            mgScore -= VulnerableToMinorOrPawnPenaltyMG;
            egScore -= VulnerableToMinorOrPawnPenaltyEG;
        }
        if (rookQueenBattery(bitboards, color, square))
        {
            mgScore += RookQueenBatteryBonusMG;
            egScore += RookQueenBatteryBonusEG;
        }
    }

    // Połączone wieże
    if (rookCount >= 2 && areRooksConnected(bitboards, color))
    {
        mgScore += RooksConnectedBonusMG;
        egScore += RooksConnectedBonusEG;
    }

    return {mgScore, egScore};
}

std::pair<int, int> RookEvaluation::evaluatePair(const Board& board, const Bitboards& bitboards, ChessColor color)
{
    return calculateRookScorePair(board, bitboards, color);
}

int RookEvaluation::evaluate(const Board& board, const Bitboards& bitboards, ChessColor color)
{
    const auto [mg, eg] = evaluatePair(board, bitboards, color);
    const int phase = Evaluation::gamePhase(bitboards);
    return (mg * phase + eg * (24 - phase)) / 24;
}
