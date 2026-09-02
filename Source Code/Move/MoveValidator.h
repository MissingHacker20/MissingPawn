#pragma once

#include "Board.h"
#include "MoveList.h"
#include "Foundation/Bitboard.h"

class MoveValidator
{
public:

    struct CheckInfo
    {
        Bitboard checkers = 0;           // Wszystkie figury dające szach
        Bitboard pinned = 0;             // Nasze figury przybite do króla
        Bitboard pinRays[64];            // Dla każdej przybitej figury: pola na których może się ruszyć
        Square kingSquare = Square::None;
        bool inCheck = false;
        bool doubleCheck = false;
        Bitboard enemyAttacks = 0;       // Wszystkie pola atakowane przez przeciwnika
    };

    // Oblicza informacje o szachach i przyspieszaniach dla danej strony.
    // Zwraca strukturę CheckInfo z gotowymi maskami.
    static CheckInfo computeCheckInfo(const Board& board, ChessColor side);

    // Ocena taktyczna (SEE / wiszące figury) dla całej planszy.
    // Zwraca wynik netto (biały - czarny): kara za "wiszące" figury
    // strony, której figura należy.
    static int evaluateTactics(
        const Board& board
    );

    // Static Exchange Evaluation (SEE): zwraca bilans wymiany na danym polu
    // (np. -2200 gdy skoczek 3200 jest bity pionkiem, a my odbijamy pionka 1000).
    // Dodatni = wymiana korzystna dla strony do ruchu; ujemna = stratna;
    // 0 = wyrównana.
    static int see(
        const Board& board,
        Square square);

    static void updatePieceBitboards(
        Board& board,
        const MoveList& legalMoves);

    // Legacy - kept for debug/validation only
    static bool isKingInCheck(
        const Board& board,
        ChessColor side
    );

    static bool isSquareAttacked(
        const Board& board,
        Square square,
        ChessColor attacker
    );

static ChessColor oppositeColor(ChessColor color);

// Legacy functions - kept for compatibility but should not be used in new code
static void filterLegalMoves(Board& board, MoveList& moveList);
static bool isMoveLegal(Board& board, const Move& move);

private:

    static Square findKing(
        const Board& board,
        ChessColor side
    );

    static int pieceValue(Piece piece);

 // Zwraca najtańszą figurę danego koloru, która atakuje pole.
    // Zapisuje jej pole w `fromSquare`. Zwraca Piece::None, jeśli nie ma.
    static Piece leastValuableAttacker(
        const Board& board,
        Square square,
        ChessColor attacker,
        Square& fromSquare);

    // Rekurencyjna część SEE. Operuje na kopii planszy (board),
    // która jest modyfikowana w trakcie symulacji wymiany.
    static int seeRecursive(
        Board& board,
        Square square,
        ChessColor sideToMove,
        int value);

    // Sprawdza, czy figura na polu `from` ma czysty promień do (targetFile,
    // targetRank) po linii prostej (wiersz/kolumna).
    static bool isStraightAttacker(
        const Board& board,
        Square from,
        int targetFile,
        int targetRank);

    // Sprawdza, czy figura na polu `from` ma czysty promień do (targetFile,
    // targetRank) po przekątnej.
    static bool isDiagonalAttacker(
        const Board& board,
        Square from,
        int targetFile,
        int targetRank);
};
