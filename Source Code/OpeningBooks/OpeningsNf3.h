#pragma once

#include <vector>

#include "OpeningBooks/BookEntry.h"

// Generuje wpisy książki dla otwarć 1.Nf3
inline std::vector<BookEntry> getNf3Openings(Board& board)
{
    std::vector<BookEntry> entries;

    // 1.Nf3
    addBookEntry(entries, board,
        Move(Square::G1, Square::F3, Piece::WhiteKnight, MoveFlag::Quiet), 500);

    // Wykonaj 1.Nf3
    {
        UndoInfo undo;
        board.makeMove(Move(Square::G1, Square::F3, Piece::WhiteKnight, MoveFlag::Quiet), undo);
    }

    // Odpowiedzi czarnych na 1.Nf3
    addBookEntry(entries, board,
        Move(Square::D7, Square::D5, Piece::BlackPawn, MoveFlag::DoublePawnPush), 500);  // 1...d5
    addBookEntry(entries, board,
        Move(Square::G8, Square::F6, Piece::BlackKnight, MoveFlag::Quiet), 400);          // 1...Nf6
    addBookEntry(entries, board,
        Move(Square::C7, Square::C5, Piece::BlackPawn, MoveFlag::DoublePawnPush), 300);  // 1...c5
    addBookEntry(entries, board,
        Move(Square::G7, Square::G6, Piece::BlackPawn, MoveFlag::Quiet), 200);           // 1...g6

    // Reti: 1.Nf3 d5 2.g3
    {
        UndoInfo undo;
        board.makeMove(Move(Square::D7, Square::D5, Piece::BlackPawn, MoveFlag::DoublePawnPush), undo);
    }

    addBookEntry(entries, board,
        Move(Square::G2, Square::G3, Piece::WhitePawn, MoveFlag::Quiet), 400);  // 2.g3
    addBookEntry(entries, board,
        Move(Square::C2, Square::C4, Piece::WhitePawn, MoveFlag::DoublePawnPush), 300);  // 2.c4

    // 1.Nf3 Nf6 2.g3
    board.setStartPosition();
    {
        UndoInfo undo;
        board.makeMove(Move(Square::G1, Square::F3, Piece::WhiteKnight, MoveFlag::Quiet), undo);
        board.makeMove(Move(Square::G8, Square::F6, Piece::BlackKnight, MoveFlag::Quiet), undo);
    }

    addBookEntry(entries, board,
        Move(Square::G2, Square::G3, Piece::WhitePawn, MoveFlag::Quiet), 300);

    // 1.Nf3 c5 2.c4
    board.setStartPosition();
    {
        UndoInfo undo;
        board.makeMove(Move(Square::G1, Square::F3, Piece::WhiteKnight, MoveFlag::Quiet), undo);
        board.makeMove(Move(Square::C7, Square::C5, Piece::BlackPawn, MoveFlag::DoublePawnPush), undo);
    }

    addBookEntry(entries, board,
        Move(Square::C2, Square::C4, Piece::WhitePawn, MoveFlag::DoublePawnPush), 250);

    // Przywróć początkową pozycję
    board.setStartPosition();

    return entries;
}
