#pragma once

#include <vector>

#include "OpeningBooks/BookEntry.h"

// Generuje wpisy książki dla otwarć 1.e4
inline std::vector<BookEntry> getE4Openings(Board& board)
{
    std::vector<BookEntry> entries;

    // 1.e4 - z pozycji startowej
    addBookEntry(entries, board,
        Move(Square::E2, Square::E4, Piece::WhitePawn, MoveFlag::DoublePawnPush), 1000);

    // Wykonaj 1.e4
    {
        UndoInfo undo;
        board.makeMove(Move(Square::E2, Square::E4, Piece::WhitePawn, MoveFlag::DoublePawnPush), undo);
    }

    // Odpowiedzi czarnych na 1.e4
    addBookEntry(entries, board,
        Move(Square::E7, Square::E5, Piece::BlackPawn, MoveFlag::DoublePawnPush), 1000);  // 1...e5
    addBookEntry(entries, board,
        Move(Square::C7, Square::C5, Piece::BlackPawn, MoveFlag::DoublePawnPush), 900);   // 1...c5 - Sycylijska
    addBookEntry(entries, board,
        Move(Square::E7, Square::E6, Piece::BlackPawn, MoveFlag::Quiet), 400);            // 1...e6 - Francuska
    addBookEntry(entries, board,
        Move(Square::C7, Square::C6, Piece::BlackPawn, MoveFlag::Quiet), 350);            // 1...c6 - Caro-Kann
    addBookEntry(entries, board,
        Move(Square::D7, Square::D6, Piece::BlackPawn, MoveFlag::Quiet), 200);            // 1...d6 - Pirc
    addBookEntry(entries, board,
        Move(Square::G8, Square::F6, Piece::BlackKnight, MoveFlag::Quiet), 150);          // 1...Nf6 - Alechin
    addBookEntry(entries, board,
        Move(Square::G7, Square::G6, Piece::BlackPawn, MoveFlag::Quiet), 180);            // 1...g6 - Nowoczesna

    // Po 1.e4 e5 2.Nf3 - wykonaj 1...e5
    {
        UndoInfo undo;
        board.makeMove(Move(Square::E7, Square::E5, Piece::BlackPawn, MoveFlag::DoublePawnPush), undo);
    }

    addBookEntry(entries, board,
        Move(Square::G1, Square::F3, Piece::WhiteKnight, MoveFlag::Quiet), 900);  // 2.Nf3

    // Wykonaj 2.Nf3
    {
        UndoInfo undo;
        board.makeMove(Move(Square::G1, Square::F3, Piece::WhiteKnight, MoveFlag::Quiet), undo);
    }

    // Odpowiedzi czarnych na 2.Nf3
    addBookEntry(entries, board,
        Move(Square::B8, Square::C6, Piece::BlackKnight, MoveFlag::Quiet), 900);  // 2...Nc6
    addBookEntry(entries, board,
        Move(Square::D7, Square::D6, Piece::BlackPawn, MoveFlag::Quiet), 200);   // 2...d6 (Philidor)

    // Po 1.e4 e5 2.Nf3 Nc6 3.Bb5
    {
        UndoInfo undo;
        board.makeMove(Move(Square::B8, Square::C6, Piece::BlackKnight, MoveFlag::Quiet), undo);
    }

    addBookEntry(entries, board,
        Move(Square::F1, Square::B5, Piece::WhiteBishop, MoveFlag::Quiet), 800);  // 3.Bb5 (Hiszpańska)
    addBookEntry(entries, board,
        Move(Square::F1, Square::C4, Piece::WhiteBishop, MoveFlag::Quiet), 600);  // 3.Bc4 (Włoska)
    addBookEntry(entries, board,
        Move(Square::D2, Square::D4, Piece::WhitePawn, MoveFlag::DoublePawnPush), 300);  // 3.d4 (Szkocka)

    // Sycylijska: 1.e4 c5 2.Nf3
    // Cofnij do pozycji po 1.e4
    board.setStartPosition();
    {
        UndoInfo undo;
        board.makeMove(Move(Square::E2, Square::E4, Piece::WhitePawn, MoveFlag::DoublePawnPush), undo);
        board.makeMove(Move(Square::C7, Square::C5, Piece::BlackPawn, MoveFlag::DoublePawnPush), undo);
    }

    addBookEntry(entries, board,
        Move(Square::G1, Square::F3, Piece::WhiteKnight, MoveFlag::Quiet), 900);  // 2.Nf3

    {
        UndoInfo undo;
        board.makeMove(Move(Square::G1, Square::F3, Piece::WhiteKnight, MoveFlag::Quiet), undo);
    }

    // Odpowiedzi na 1.e4 c5 2.Nf3
    addBookEntry(entries, board,
        Move(Square::D7, Square::D6, Piece::BlackPawn, MoveFlag::Quiet), 600);  // 2...d6
    addBookEntry(entries, board,
        Move(Square::B8, Square::C6, Piece::BlackKnight, MoveFlag::Quiet), 500);  // 2...Nc6
    addBookEntry(entries, board,
        Move(Square::E7, Square::E6, Piece::BlackPawn, MoveFlag::Quiet), 400);  // 2...e6

    // Francuska: 1.e4 e6 2.d4
    board.setStartPosition();
    {
        UndoInfo undo;
        board.makeMove(Move(Square::E2, Square::E4, Piece::WhitePawn, MoveFlag::DoublePawnPush), undo);
        board.makeMove(Move(Square::E7, Square::E6, Piece::BlackPawn, MoveFlag::Quiet), undo);
    }

    addBookEntry(entries, board,
        Move(Square::D2, Square::D4, Piece::WhitePawn, MoveFlag::DoublePawnPush), 700);

    // Caro-Kann: 1.e4 c6 2.d4
    board.setStartPosition();
    {
        UndoInfo undo;
        board.makeMove(Move(Square::E2, Square::E4, Piece::WhitePawn, MoveFlag::DoublePawnPush), undo);
        board.makeMove(Move(Square::C7, Square::C6, Piece::BlackPawn, MoveFlag::Quiet), undo);
    }

    addBookEntry(entries, board,
        Move(Square::D2, Square::D4, Piece::WhitePawn, MoveFlag::DoublePawnPush), 600);

    // Przywróć początkową pozycję
    board.setStartPosition();

    return entries;
}
