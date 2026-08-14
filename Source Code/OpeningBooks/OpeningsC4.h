#pragma once

#include <vector>

#include "OpeningBooks/BookEntry.h"

// Generuje wpisy książki dla otwarć 1.c4 (Angielskie)
inline std::vector<BookEntry> getC4Openings(Board& board)
{
    std::vector<BookEntry> entries;

    // 1.c4
    addBookEntry(entries, board,
        Move(Square::C2, Square::C4, Piece::WhitePawn, MoveFlag::DoublePawnPush), 400);

    // Wykonaj 1.c4
    {
        UndoInfo undo;
        board.makeMove(Move(Square::C2, Square::C4, Piece::WhitePawn, MoveFlag::DoublePawnPush), undo);
    }

    // Odpowiedzi czarnych na 1.c4
    addBookEntry(entries, board,
        Move(Square::E7, Square::E5, Piece::BlackPawn, MoveFlag::DoublePawnPush), 300);  // 1...e5
    addBookEntry(entries, board,
        Move(Square::C7, Square::C5, Piece::BlackPawn, MoveFlag::DoublePawnPush), 200);  // 1...c5 (Symetryczna)
    addBookEntry(entries, board,
        Move(Square::G8, Square::F6, Piece::BlackKnight, MoveFlag::Quiet), 300);          // 1...Nf6
    addBookEntry(entries, board,
        Move(Square::E7, Square::E6, Piece::BlackPawn, MoveFlag::Quiet), 250);            // 1...e6
    addBookEntry(entries, board,
        Move(Square::D7, Square::D5, Piece::BlackPawn, MoveFlag::DoublePawnPush), 200);  // 1...d5

    // 1.c4 e5 (Angielska - klasyczna)
    {
        UndoInfo undo;
        board.makeMove(Move(Square::E7, Square::E5, Piece::BlackPawn, MoveFlag::DoublePawnPush), undo);
    }

    addBookEntry(entries, board,
        Move(Square::G1, Square::F3, Piece::WhiteKnight, MoveFlag::Quiet), 300);  // 2.Nf3
    addBookEntry(entries, board,
        Move(Square::B1, Square::C3, Piece::WhiteKnight, MoveFlag::Quiet), 250);  // 2.Nc3

    // 1.c4 c5 2.Nf3 (Symetryczna)
    board.setStartPosition();
    {
        UndoInfo undo;
        board.makeMove(Move(Square::C2, Square::C4, Piece::WhitePawn, MoveFlag::DoublePawnPush), undo);
        board.makeMove(Move(Square::C7, Square::C5, Piece::BlackPawn, MoveFlag::DoublePawnPush), undo);
    }

    addBookEntry(entries, board,
        Move(Square::G1, Square::F3, Piece::WhiteKnight, MoveFlag::Quiet), 200);

    // 1.c4 e5 2.Nc3 Nf6
    board.setStartPosition();
    {
        UndoInfo undo;
        board.makeMove(Move(Square::C2, Square::C4, Piece::WhitePawn, MoveFlag::DoublePawnPush), undo);
        board.makeMove(Move(Square::E7, Square::E5, Piece::BlackPawn, MoveFlag::DoublePawnPush), undo);
        board.makeMove(Move(Square::B1, Square::C3, Piece::WhiteKnight, MoveFlag::Quiet), undo);
    }

    addBookEntry(entries, board,
        Move(Square::G8, Square::F6, Piece::BlackKnight, MoveFlag::Quiet), 250);

    // Przywróć początkową pozycję
    board.setStartPosition();

    return entries;
}
