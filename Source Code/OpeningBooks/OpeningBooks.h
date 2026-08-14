#pragma once

#include <vector>
#include <cstdlib>
#include <ctime>

#include "OpeningBooks/BookEntry.h"
#include "OpeningBooks/OpeningsE4.h"
#include "OpeningBooks/OpeningsD4.h"
#include "OpeningBooks/OpeningsNf3.h"
#include "OpeningBooks/OpeningsC4.h"
#include "OpeningBooks/OpeningsF4.h"
#include "OpeningBooks/OpeningsSide.h"

// Główna baza książki otwarć - agreguje wszystkie kategorie
inline std::vector<BookEntry> buildOpeningBook()
{
    Board board;
    std::vector<BookEntry> book;

    // Dodaj wszystkie kategorie otwarć
    auto e4 = getE4Openings(board);
    auto d4 = getD4Openings(board);
    auto nf3 = getNf3Openings(board);
    auto c4 = getC4Openings(board);
    auto f4 = getF4Openings(board);
    auto side = getSideOpenings(board);

    book.insert(book.end(), e4.begin(), e4.end());
    book.insert(book.end(), d4.begin(), d4.end());
    book.insert(book.end(), nf3.begin(), nf3.end());
    book.insert(book.end(), c4.begin(), c4.end());
    book.insert(book.end(), f4.begin(), f4.end());
    book.insert(book.end(), side.begin(), side.end());

    return book;
}

// Inicjalizacja książki
inline void initializeBook()
{
    std::srand(static_cast<unsigned>(std::time(nullptr)));
}

// Sprawdza czy książka ma ruch dla danej pozycji
inline bool hasBookMove(const std::vector<BookEntry>& book, uint64_t zobristKey)
{
    for (const auto& entry : book)
    {
        if (entry.key == zobristKey)
        {
            return true;
        }
    }
    return false;
}

// Pobiera losowy ruch z książki według wagi
inline Move getBookMove(const std::vector<BookEntry>& book, const Board& board)
{
    uint64_t key = board.getZobristKey();

    // Zbierz wszystkie pasujące ruchy
    std::vector<const BookEntry*> candidates;
    int totalWeight = 0;

    for (const auto& entry : book)
    {
        if (entry.key == key)
        {
            candidates.push_back(&entry);
            totalWeight += entry.weight;
        }
    }

    if (candidates.empty())
    {
        return Move();  // Pusty ruch
    }

    // Wybierz losowo według wagi
    int random = std::rand() % totalWeight;
    int cumulative = 0;

    for (const auto* entry : candidates)
    {
        cumulative += entry->weight;
        if (random < cumulative)
        {
            return codeToMove(board, entry->encodedMove);
        }
    }

    // Fallback: pierwszy pasujący
    return codeToMove(board, candidates[0]->encodedMove);
}
