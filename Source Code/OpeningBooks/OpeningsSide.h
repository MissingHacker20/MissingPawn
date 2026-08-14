#pragma once

#include <vector>

#include "OpeningBooks/BookEntry.h"

// Generuje wpisy książki dla otwarć bocznych (1.g3, 1.b3)
inline std::vector<BookEntry> getSideOpenings(Board& board)
{
    std::vector<BookEntry> entries;

    // 1.g3 (King's Fianchetto)
    addBookEntry(entries, board,
        Move(Square::G2, Square::G3, Piece::WhitePawn, MoveFlag::Quiet), 200);

    // Wykonaj 1.g3
    {
        UndoInfo undo;
        board.makeMove(Move(Square::G2, Square::G3, Piece::WhitePawn, MoveFlag::Quiet), undo);
    }

    // Odpowiedzi czarnych na 1.g3
    addBookEntry(entries, board,
        Move(Square::D7, Square::D5, Piece::BlackPawn, MoveFlag::DoublePawnPush), 200);  // 1...d5
    addBookEntry(entries, board,
        Move(Square::E7, Square::E5, Piece::BlackPawn, MoveFlag::DoublePawnPush), 150);  // 1...e5
    addBookEntry(entries, board,
        Move(Square::G8, Square::F6, Piece::BlackKnight, MoveFlag::Quiet), 150);          // 1...Nf6

    // 1.b3 (Nimzo-Larsen)
    board.setStartPosition();
    addBookEntry(entries, board,
        Move(Square::B2, Square::B3, Piece::WhitePawn, MoveFlag::Quiet), 100);

    // Wykonaj 1.b3
    {
        UndoInfo undo;
        board.makeMove(Move(Square::B2, Square::B3, Piece::WhitePawn, MoveFlag::Quiet), undo);
    }

    // Odpowiedzi czarnych na 1.b3
    addBookEntry(entries, board,
        Move(Square::D7, Square::D5, Piece::BlackPawn, MoveFlag::DoublePawnPush), 100);  // 1...d5
    addBookEntry(entries, board,
        Move(Square::E7, Square::E5, Piece::BlackPawn, MoveFlag::DoublePawnPush), 80);   // 1...e5

    // Przywróć początkową pozycję
    board.setStartPosition();

    return entries;
}
