#pragma once

#include <array>
#include <vector>

#include "Bitboard.h"
#include "Color.h"
#include "Move.h"
#include "Piece.h"
#include "Square.h"

class Board
{
public:

    Board();

    void clear();

    void updateZobristKey();

    bool hasPiece(Piece piece, Square square) const;

    void removePiece(Piece piece, Square square);

    void setPiece(Piece piece, Square square);

    void setStartPosition();

    void print() const;

    void movePiece(Piece piece, Square from, Square to);

    void makeMove(const Move& move, UndoInfo& undoInfo);

    void undoMove(const Move& move, const UndoInfo& undoInfo);

    void makeNullMove(
    UndoInfo& undoInfo);

    void undoNullMove(
    const UndoInfo& undoInfo);

    Piece pieceAt(Square square) const;

    void updateOccupancy();

    Bitboard getBitboard(Piece piece) const;

    Bitboard getOccupancy(ChessColor color) const;

    Bitboard getAllOccupancy() const;

    ChessColor getSideToMove() const;

    Square getEnPassantSquare() const;

    uint8_t getCastlingRights() const;

    int getHalfmoveClock() const;

    uint64_t getZobristKey() const;

    friend class Fen;

    int getFullmoveNumber() const;

    Square getKingSquare(
    ChessColor color) const;

private:

    uint64_t zobristKey = 0;

    ChessColor sideToMove = ChessColor::White;

    Bitboard whiteOccupancy = 0;
    Bitboard blackOccupancy = 0;
    Bitboard allOccupancy = 0;

    uint8_t castlingRights = 0;

    Square enPassantSquare = Square::None;

    std::array<Piece, 64> mailbox;

    std::array<Bitboard, static_cast<int>(Piece::Count)> bitboards;

    int halfmoveClock = 0;

    int fullmoveNumber = 1;
};