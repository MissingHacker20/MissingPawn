/*
  This file contains code derived from Stockfish.

  Copyright (C) 2004-2026 The Stockfish developers

  Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published
  by the Free Software Foundation, either version 3 of the License,
  or (at your option) any later version.

  This file has been modified and adapted for the MissingPawn chess
  engine.

  Modifications:
  Copyright (C) 2026 Kacper Wieczorek
*/

#include "Game/Fen.h"

#include "Foundation/Board.h"

#include <array>
#include <charconv>
#include <sstream>

namespace
{
Piece pieceFromFen(char character)
{
    switch (character)
    {
    case 'P': return Piece::WhitePawn;
    case 'N': return Piece::WhiteKnight;
    case 'B': return Piece::WhiteBishop;
    case 'R': return Piece::WhiteRook;
    case 'Q': return Piece::WhiteQueen;
    case 'K': return Piece::WhiteKing;
    case 'p': return Piece::BlackPawn;
    case 'n': return Piece::BlackKnight;
    case 'b': return Piece::BlackBishop;
    case 'r': return Piece::BlackRook;
    case 'q': return Piece::BlackQueen;
    case 'k': return Piece::BlackKing;
    default:  return Piece::None;
    }
}

bool parseNonNegativeInt(const std::string& text, int& value)
{
    const char* begin = text.data();
    const char* end = begin + text.size();
    const auto [position, error] = std::from_chars(begin, end, value);
    return error == std::errc{} && position == end && value >= 0;
}
}

bool Fen::load(Board& board, const std::string& fen)
{
    std::istringstream input(fen);
    std::array<std::string, 6> fields;

    for (std::string& field : fields)
    {
        if (!(input >> field))
        {
            return false;
        }
    }

    std::string extra;
    if (input >> extra)
    {
        return false;
    }

    board.clear();

    int rank = 7;
    int file = 0;
    for (char character : fields[0])
    {
        if (character == '/')
        {
            if (file != 8 || rank == 0)
            {
                return false;
            }

            --rank;
            file = 0;
            continue;
        }

        if (character >= '1' && character <= '8')
        {
            file += character - '0';
        }
        else
        {
            const Piece piece = pieceFromFen(character);
            if (piece == Piece::None || file >= 8)
            {
                return false;
            }

            board.setPiece(piece, static_cast<Square>(rank * 8 + file));
            ++file;
        }

        if (file > 8)
        {
            return false;
        }
    }

    if (rank != 0 || file != 8)
    {
        return false;
    }

    if (fields[1] == "w")
    {
        board.sideToMove = ChessColor::White;
    }
    else if (fields[1] == "b")
    {
        board.sideToMove = ChessColor::Black;
    }
    else
    {
        return false;
    }

    board.castlingRights = 0;
    if (fields[2] != "-")
    {
        for (char right : fields[2])
        {
            uint8_t bit = 0;
            switch (right)
            {
            case 'K': bit = 0b0001; break;
            case 'Q': bit = 0b0010; break;
            case 'k': bit = 0b0100; break;
            case 'q': bit = 0b1000; break;
            default: return false;
            }

            if (board.castlingRights & bit)
            {
                return false;
            }
            board.castlingRights |= bit;
        }
    }

    board.enPassantSquare = Square::None;
    if (fields[3] != "-")
    {
        const Square square = stringToSquare(fields[3]);
        const int rankIndex = static_cast<int>(square) / 8;
        const int expectedRank = board.getSideToMove() == ChessColor::White ? 5 : 2;
        if (square == Square::None || rankIndex != expectedRank)
        {
            return false;
        }

        // Stockfish records an en-passant square only when a legal EP capture
        // exists. FENs often contain a stale target after a double pawn push;
        // such a target has no effect on legal move generation and must not
        // prevent an otherwise valid position from being loaded.
        const bool white = board.getSideToMove() == ChessColor::White;
        const int squareIndex = static_cast<int>(square);
        const int capturedIndex = squareIndex + (white ? -8 : 8);
        const int capturerRank = squareIndex / 8 + (white ? -1 : 1);
        const Piece capturedPawn = white ? Piece::BlackPawn : Piece::WhitePawn;
        const Piece capturerPawn = white ? Piece::WhitePawn : Piece::BlackPawn;
        bool hasCapturingPawn = false;
        for (int fileDelta : {-1, 1})
        {
            const int file = squareIndex % 8 + fileDelta;
            if (file >= 0 && file < 8 && capturerRank >= 0 && capturerRank < 8 &&
                board.pieceAt(static_cast<Square>(capturerRank * 8 + file)) == capturerPawn)
                hasCapturingPawn = true;
        }

        if (board.pieceAt(square) == Piece::None && capturedIndex >= 0 && capturedIndex < 64 &&
            board.pieceAt(static_cast<Square>(capturedIndex)) == capturedPawn && hasCapturingPawn)
            board.enPassantSquare = square;
    }

    if (!parseNonNegativeInt(fields[4], board.halfmoveClock) ||
        !parseNonNegativeInt(fields[5], board.fullmoveNumber) ||
        board.fullmoveNumber == 0)
    {
        return false;
    }

    board.updateOccupancy();
    board.updateZobristKey();
    return true;
}
