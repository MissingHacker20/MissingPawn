#pragma once

#include <vector>

#include "OpeningBooks/BookEntry.h"

// Generuje wpisy książki dla otwarć 1.d4
inline std::vector<BookEntry> getD4Openings(Board& board)
{
    std::vector<BookEntry> entries;

    // 1.d4
    addBookEntry(entries, board,
        Move(Square::D2, Square::D4, Piece::WhitePawn, MoveFlag::DoublePawnPush), 900);

    // Wykonaj 1.d4
    {
        UndoInfo undo;
        board.makeMove(Move(Square::D2, Square::D4, Piece::WhitePawn, MoveFlag::DoublePawnPush), undo);
    }

    // Odpowiedzi czarnych na 1.d4
    addBookEntry(entries, board,
        Move(Square::D7, Square::D5, Piece::BlackPawn, MoveFlag::DoublePawnPush), 1000);  // 1...d5
    addBookEntry(entries, board,
        Move(Square::G8, Square::F6, Piece::BlackKnight, MoveFlag::Quiet), 800);          // 1...Nf6 (Indyjska)
    addBookEntry(entries, board,
        Move(Square::F7, Square::F5, Piece::BlackPawn, MoveFlag::DoublePawnPush), 200);   // 1...f5 (Holenderska)
    addBookEntry(entries, board,
        Move(Square::E7, Square::E6, Piece::BlackPawn, MoveFlag::Quiet), 300);            // 1...e6
    addBookEntry(entries, board,
        Move(Square::C7, Square::C5, Piece::BlackPawn, MoveFlag::DoublePawnPush), 250);  // 1...c5 (Benoni)
    addBookEntry(entries, board,
        Move(Square::G7, Square::G6, Piece::BlackPawn, MoveFlag::Quiet), 300);            // 1...g6

    // Gambit Hetmański: 1.d4 d5 2.c4
    {
        UndoInfo undo;
        board.makeMove(Move(Square::D7, Square::D5, Piece::BlackPawn, MoveFlag::DoublePawnPush), undo);
    }

    addBookEntry(entries, board,
        Move(Square::C2, Square::C4, Piece::WhitePawn, MoveFlag::DoublePawnPush), 900);

    {
        UndoInfo undo;
        board.makeMove(Move(Square::C2, Square::C4, Piece::WhitePawn, MoveFlag::DoublePawnPush), undo);
    }

    // Odpowiedzi na 1.d4 d5 2.c4
    addBookEntry(entries, board,
        Move(Square::E7, Square::E6, Piece::BlackPawn, MoveFlag::Quiet), 500);      // 2...e6
    addBookEntry(entries, board,
        Move(Square::C7, Square::C6, Piece::BlackPawn, MoveFlag::Quiet), 500);      // 2...c6 (Słowiańska)
    addBookEntry(entries, board,
        Move(Square::D5, Square::C4, Piece::BlackPawn, MoveFlag::Capture), 400);    // 2...dxc4 (Przyjęty)

    // Indyjska: 1.d4 Nf6 2.c4
    board.setStartPosition();
    {
        UndoInfo undo;
        board.makeMove(Move(Square::D2, Square::D4, Piece::WhitePawn, MoveFlag::DoublePawnPush), undo);
        board.makeMove(Move(Square::G8, Square::F6, Piece::BlackKnight, MoveFlag::Quiet), undo);
    }

    addBookEntry(entries, board,
        Move(Square::C2, Square::C4, Piece::WhitePawn, MoveFlag::DoublePawnPush), 800);

    // Królewsko-Indyjska: 1.d4 Nf6 2.c4 g6
    {
        UndoInfo undo;
        board.makeMove(Move(Square::C2, Square::C4, Piece::WhitePawn, MoveFlag::DoublePawnPush), undo);
        board.makeMove(Move(Square::G7, Square::G6, Piece::BlackPawn, MoveFlag::Quiet), undo);
    }

    addBookEntry(entries, board,
        Move(Square::B1, Square::C3, Piece::WhiteKnight, MoveFlag::Quiet), 500);

    // Hetmańsko-Indyjska: 1.d4 Nf6 2.c4 e6
    board.setStartPosition();
    {
        UndoInfo undo;
        board.makeMove(Move(Square::D2, Square::D4, Piece::WhitePawn, MoveFlag::DoublePawnPush), undo);
        board.makeMove(Move(Square::G8, Square::F6, Piece::BlackKnight, MoveFlag::Quiet), undo);
        board.makeMove(Move(Square::C2, Square::C4, Piece::WhitePawn, MoveFlag::DoublePawnPush), undo);
        board.makeMove(Move(Square::E7, Square::E6, Piece::BlackPawn, MoveFlag::Quiet), undo);
    }

    addBookEntry(entries, board,
        Move(Square::B1, Square::C3, Piece::WhiteKnight, MoveFlag::Quiet), 500);

    // Nimzo-Indyjska: 1.d4 Nf6 2.c4 e6 3.Nc3 Bb4
    {
        UndoInfo undo;
        board.makeMove(Move(Square::B1, Square::C3, Piece::WhiteKnight, MoveFlag::Quiet), undo);
    }

    addBookEntry(entries, board,
        Move(Square::F8, Square::B4, Piece::BlackBishop, MoveFlag::Quiet), 400);

    // Królewsko-Indyjska: 1.d4 Nf6 2.c4 g6 3.Nc3 Bg7
    board.setStartPosition();
    {
        UndoInfo undo;
        board.makeMove(Move(Square::D2, Square::D4, Piece::WhitePawn, MoveFlag::DoublePawnPush), undo);
        board.makeMove(Move(Square::G8, Square::F6, Piece::BlackKnight, MoveFlag::Quiet), undo);
        board.makeMove(Move(Square::C2, Square::C4, Piece::WhitePawn, MoveFlag::DoublePawnPush), undo);
        board.makeMove(Move(Square::G7, Square::G6, Piece::BlackPawn, MoveFlag::Quiet), undo);
        board.makeMove(Move(Square::B1, Square::C3, Piece::WhiteKnight, MoveFlag::Quiet), undo);
    }

    addBookEntry(entries, board,
        Move(Square::F8, Square::G7, Piece::BlackBishop, MoveFlag::Quiet), 500);

    // Przywróć początkową pozycję
    board.setStartPosition();

    return entries;
}
