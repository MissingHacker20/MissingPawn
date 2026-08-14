#pragma once

#include "Board.h"
#include "MoveList.h"

class MoveValidator
{
public:

    static void filterLegalMoves(
        Board& board,
        MoveList& moveList
    );
    
    static bool isMoveLegal(
        Board& board,
        const Move& move
    );

    static bool isKingInCheck(
        const Board& board,
        ChessColor side
    );

static bool isSquareAttacked(
        const Board& board,
        Square square,
        ChessColor attacker
    );

    // Ocena taktyczna (SEE / wiszące figury) dla całej planszy.
    // Zwraca wynik netto (biały - czarny): kara za "wiszące" figury
    // strony, której figura należy.
    static int evaluateTactics(
        const Board& board
    );

    // Static Exchange Evaluation (SEE): zwraca bilans wymiany na danym polu
    // (np. -220 gdy koń 320 jest bity pionkiem, a my odbijamy pionka 100).
    // Dodatni = wymiana korzystna dla strony do ruchu; ujemna = stratna;
    // 0 = wyrównana.
    static int see(
        const Board& board,
        Square square);

    static void updatePieceBitboards(
        Board& board,
        const MoveList& legalMoves);

private:

    static ChessColor oppositeColor(ChessColor color);

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
