#pragma once

#include <vector>

#include "OpeningBooks/BookEntry.h"

// Generuje wpisy książki dla otwarć 1.f4 (Bird's Opening)
inline std::vector<BookEntry> getF4Openings(Board& board)
{
    std::vector<BookEntry> entries;

    // 1.f4
    addBookEntry(entries, board,
        Move(Square::F2, Square::F4, Piece::WhitePawn, MoveFlag::DoublePawnPush), 150);

    // Wykonaj 1.f4
    {
        UndoInfo undo;
        board.makeMove(Move(Square::F2, Square::F4, Piece::WhitePawn, MoveFlag::DoublePawnPush), undo);
    }

    // Odpowiedzi czarnych na 1.f4
    addBookEntry(entries, board,
        Move(Square::D7, Square::D5, Piece::BlackPawn, MoveFlag::DoublePawnPush), 150);  // 1...d5
    addBookEntry(entries, board,
        Move(Square::E7, Square::E5, Piece::BlackPawn, MoveFlag::DoublePawnPush), 100);  // 1...e5 (From's Gambit)
    addBookEntry(entries, board,
        Move(Square::G8, Square::F6, Piece::BlackKnight, MoveFlag::Quiet), 100);          // 1...Nf6

    // Przywróć początkową pozycję
    board.setStartPosition();

    return entries;
}
