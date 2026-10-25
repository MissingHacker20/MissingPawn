/*
 * MissingPawn
 * Copyright (C) 2026 Kacper Wieczorek
 *
 * This file is part of MissingPawn.
 *
 * Licensed under the GNU General Public License v3 or later.
 * See LICENSE for details.
 */

#pragma once
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <random>
#include <string>
#include <vector>

#include "Foundation/Board.h"
#include "Foundation/Move.h"
#include "Foundation/Piece.h"
#include "Foundation/Square.h"
#include "Move/MoveGenerator.h"
#include "Move/MoveValidator.h"

struct PolyglotBookEntry
{
    uint64_t key = 0;
    uint16_t encodedMove = 0;
    uint16_t weight = 0;
};

namespace PolyglotBook
{
    inline const uint64_t random64[781] = {
#include "OpeningBooks/PolyglotRandom.inc"
    };

    inline uint64_t getKey(const Board &board)
    {
        uint64_t key = 0;
        for (int square = 0; square < 64; ++square)
        {
            const Piece piece = board.pieceAt(static_cast<Square>(square));
            if (piece == Piece::None)
                continue;

            const bool white = piece <= Piece::WhiteKing;
            const int pieceType = (static_cast<int>(piece) - 1) % 6;
            const int polyglotPiece = pieceType * 2 + (white ? 1 : 0);
            key ^= random64[polyglotPiece * 64 + square];
        }

        if (board.getSideToMove() == ChessColor::White)
            key ^= random64[780];

        const uint8_t rights = board.getCastlingRights();
        // Board bits: white king-side, white queen-side, black king-side, black queen-side.
        if (rights & 1)
            key ^= random64[768];
        if (rights & 2)
            key ^= random64[769];
        if (rights & 4)
            key ^= random64[770];
        if (rights & 8)
            key ^= random64[771];

        const Square ep = board.getEnPassantSquare();
        if (ep != Square::None)
        {
            const int file = static_cast<int>(ep) % 8;
            const Bitboard pawns = board.getBitboard(board.getSideToMove() == ChessColor::White
                                                         ? Piece::WhitePawn
                                                         : Piece::BlackPawn);
            const int rank = static_cast<int>(ep) / 8;
            for (int offset : {-1, 1})
            {
                const int sourceFile = file + offset;
                const int sourceRank = rank + (board.getSideToMove() == ChessColor::White ? -1 : 1);
                if (sourceFile >= 0 && sourceFile < 8 && sourceRank >= 0 && sourceRank < 8 &&
                    (pawns & (Bitboard(1) << (sourceRank * 8 + sourceFile))))
                {
                    key ^= random64[772 + file];
                    break;
                }
            }
        }
        return key;
    }

    inline bool load(const std::string &path, std::vector<PolyglotBookEntry> &entries)
    {
        std::ifstream input(path, std::ios::binary);
        if (!input)
            return false;

        std::vector<PolyglotBookEntry> loaded;
        unsigned char bytes[16];
        while (input.read(reinterpret_cast<char *>(bytes), sizeof(bytes)))
        {
            const auto readBigEndian = [&bytes](int offset, int count)
            {
                uint64_t value = 0;
                for (int i = 0; i < count; ++i)
                    value = (value << 8) | bytes[offset + i];
                return value;
            };
            loaded.push_back({readBigEndian(0, 8),
                              static_cast<uint16_t>(readBigEndian(8, 2)),
                              static_cast<uint16_t>(readBigEndian(10, 2))});
        }
        if (!input.eof())
            return false;
        std::sort(loaded.begin(), loaded.end(), [](const auto &a, const auto &b)
                  { return a.key < b.key; });
        entries = std::move(loaded);
        return true;
    }

    inline Move decodeMove(const Board &board, uint16_t encoded)
    {
        const int toFile = encoded & 7;
        const int toRank = (encoded >> 3) & 7;
        const int fromFile = (encoded >> 6) & 7;
        const int fromRank = (encoded >> 9) & 7;
        const int promotion = (encoded >> 12) & 7;
        const Square from = static_cast<Square>(fromRank * 8 + fromFile);
        const Square to = static_cast<Square>(toRank * 8 + toFile);
        const Piece piece = board.pieceAt(from);
        const Piece captured = board.pieceAt(to);

        MoveFlag flag = captured == Piece::None ? MoveFlag::Quiet : MoveFlag::Capture;
        // Polyglot encodes castling as the king moving onto the rook's square.
        // Convert it only if that rook is actually still on its original square;
        // otherwise this is an ordinary king move to a1/h1/a8/h8.
        Square engineTo = to;
        if ((piece == Piece::WhiteKing || piece == Piece::BlackKing) && fromFile == 4)
        {
            const int homeRank = piece == Piece::WhiteKing ? 0 : 7;
            if (toRank == homeRank && toFile == 7 &&
                board.pieceAt(static_cast<Square>(homeRank * 8 + 7)) ==
                    (piece == Piece::WhiteKing ? Piece::WhiteRook : Piece::BlackRook))
            {
                engineTo = static_cast<Square>(homeRank * 8 + 6);
                flag = MoveFlag::KingCastle;
            }
            else if (toRank == homeRank && toFile == 0 &&
                     board.pieceAt(static_cast<Square>(homeRank * 8)) ==
                         (piece == Piece::WhiteKing ? Piece::WhiteRook : Piece::BlackRook))
            {
                engineTo = static_cast<Square>(homeRank * 8 + 2);
                flag = MoveFlag::QueenCastle;
            }
        }
        if ((piece == Piece::WhitePawn || piece == Piece::BlackPawn) && to == board.getEnPassantSquare() && captured == Piece::None)
            flag = MoveFlag::EnPassant;

        const bool capture = captured != Piece::None || flag == MoveFlag::EnPassant;
        switch (promotion)
        {
        case 1:
            flag = capture ? MoveFlag::PromotionCaptureKnight : MoveFlag::PromotionKnight;
            break;
        case 2:
            flag = capture ? MoveFlag::PromotionCaptureBishop : MoveFlag::PromotionBishop;
            break;
        case 3:
            flag = capture ? MoveFlag::PromotionCaptureRook : MoveFlag::PromotionRook;
            break;
        case 4:
            flag = capture ? MoveFlag::PromotionCaptureQueen : MoveFlag::PromotionQueen;
            break;
        default:
            if ((piece == Piece::WhitePawn && toRank - fromRank == 2) ||
                (piece == Piece::BlackPawn && fromRank - toRank == 2))
                flag = MoveFlag::DoublePawnPush;
            break;
        }
        const Piece capturedPiece = flag == MoveFlag::EnPassant
                                        ? (piece == Piece::WhitePawn ? Piece::BlackPawn : Piece::WhitePawn)
                                        : captured;
        return Move(from, engineTo, piece, flag, capturedPiece);
    }

    inline Move getMove(const std::vector<PolyglotBookEntry> &book, Board &board, int maxPly)
    {
        const uint64_t key = getKey(board);
        MoveList legalMoves;
        MoveGenerator::generateMoves(board, legalMoves, MoveValidator::CheckInfo{});
        const auto first = std::lower_bound(book.begin(), book.end(), key,
                                            [](const PolyglotBookEntry &entry, uint64_t value)
                                            { return entry.key < value; });
        std::vector<const PolyglotBookEntry *> candidates;
        uint64_t totalWeight = 0;
        for (auto it = first; it != book.end() && it->key == key; ++it)
        {
            Move move = decodeMove(board, it->encodedMove);
            // The book stores castling as e1h1/e1a1 (or e8h8/e8a8),
            // while the engine's move generator uses e1g1/e1c1.
            if ((move.piece == Piece::WhiteKing || move.piece == Piece::BlackKing) &&
                move.from == (move.piece == Piece::WhiteKing ? Square::E1 : Square::E8) &&
                move.flag == MoveFlag::Quiet)
            {
                const int homeRank = move.piece == Piece::WhiteKing ? 0 : 7;
                if (move.to == static_cast<Square>(homeRank * 8 + 7))
                {
                    move.to = static_cast<Square>(homeRank * 8 + 6);
                    move.flag = MoveFlag::KingCastle;
                }
                else if (move.to == static_cast<Square>(homeRank * 8))
                {
                    move.to = static_cast<Square>(homeRank * 8 + 2);
                    move.flag = MoveFlag::QueenCastle;
                }
            }
            bool legal = false;
            for (int i = 0; i < legalMoves.size(); ++i)
            {
                if (legalMoves[i].from == move.from && legalMoves[i].to == move.to &&
                    legalMoves[i].flag == move.flag)
                {
                    legal = true;
                    break;
                }
            }
            if (legal && it->weight > 0 &&
                (maxPly <= 0 || (board.getFullmoveNumber() - 1) < maxPly))
            {
                candidates.push_back(&*it);
                totalWeight += it->weight;
            }
        }
        if (candidates.empty())
            return Move();

        static thread_local std::mt19937_64 rng(std::random_device{}());
        std::uniform_int_distribution<uint64_t> distribution(0, totalWeight - 1);
        uint64_t pick = distribution(rng);
        for (const auto *entry : candidates)
        {
            if (pick < entry->weight)
            {
                return decodeMove(board, entry->encodedMove);
            }
            pick -= entry->weight;
        }
        return Move();
    }
}

inline bool loadOpeningBook(const std::string &path, std::vector<PolyglotBookEntry> &book)
{
    return PolyglotBook::load(path, book);
}
inline Move getOpeningBookMove(const std::vector<PolyglotBookEntry> &book, Board &board, int maxPly)
{
    return PolyglotBook::getMove(book, board, maxPly);
}

inline int getOpeningBookMaxFullmove(const std::vector<PolyglotBookEntry> &book)
{
    if (book.empty())
        return 0;

    // Follow deterministic, weighted-majority moves from the initial position,
    // resetting to the start after a dead end. This finds deep supported lines
    // without claiming a book line through positions missing from the database.
    std::vector<PolyglotBookEntry> entries = book;
    Board board;
    board.setStartPosition();
    int maxFullmove = 0;
    int ply = 0;
    for (int step = 0; step < 512; ++step)
    {
        const uint64_t key = PolyglotBook::getKey(board);
        const auto first = std::lower_bound(entries.begin(), entries.end(), key,
            [](const PolyglotBookEntry& entry, uint64_t value) { return entry.key < value; });
        auto best = entries.end();
        uint16_t bestWeight = 0;
        for (auto it = first; it != entries.end() && it->key == key; ++it)
        {
            if (it->weight > bestWeight)
            {
                best = it;
                bestWeight = it->weight;
            }
        }
        if (best == entries.end())
        {
            if (ply == 0)
                return maxFullmove;
            board.setStartPosition();
            ply = 0;
            continue;
        }
        Move move = PolyglotBook::decodeMove(board, best->encodedMove);
        if ((move.piece == Piece::WhiteKing || move.piece == Piece::BlackKing) &&
            move.from == (move.piece == Piece::WhiteKing ? Square::E1 : Square::E8) &&
            move.flag == MoveFlag::Quiet)
        {
            const int homeRank = move.piece == Piece::WhiteKing ? 0 : 7;
            if (move.to == static_cast<Square>(homeRank * 8 + 7))
            {
                move.to = static_cast<Square>(homeRank * 8 + 6);
                move.flag = MoveFlag::KingCastle;
            }
            else if (move.to == static_cast<Square>(homeRank * 8))
            {
                move.to = static_cast<Square>(homeRank * 8 + 2);
                move.flag = MoveFlag::QueenCastle;
            }
        }
        MoveList legalMoves;
        MoveGenerator::generateMoves(board, legalMoves, MoveValidator::CheckInfo{});
        bool legal = false;
        for (int i = 0; i < legalMoves.size(); ++i)
        {
            if (legalMoves[i].from == move.from && legalMoves[i].to == move.to &&
                legalMoves[i].flag == move.flag)
            {
                legal = true;
                break;
            }
        }
        if (!legal)
        {
            board.setStartPosition();
            ply = 0;
            continue;
        }
        UndoInfo undo;
        board.makeMove(move, undo);
        ++ply;
        maxFullmove = std::max(maxFullmove, (ply + 1) / 2);
    }
    return maxFullmove;
}
