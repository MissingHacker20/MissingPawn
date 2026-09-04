#include "MoveValidator.h"
#include "Board.h"
#include "AttackTables.h"

#include <algorithm>

// (Nie używamy wersji bitboardowej tutaj - patrz computeAttackBoardFromBoard poniżej)

// Unia ataków strony `attacker` liczona z Board + AttackTables
// (używana w computeCheckInfo; ta sama semantyka co wersja bitboardowa).
static Bitboard computeAttackBoardFromBoard(const Board& board, ChessColor attacker)
{
    const Piece pawn   = (attacker == ChessColor::White) ? Piece::WhitePawn   : Piece::BlackPawn;
    const Piece knight = (attacker == ChessColor::White) ? Piece::WhiteKnight : Piece::BlackKnight;
    const Piece bishop = (attacker == ChessColor::White) ? Piece::WhiteBishop : Piece::BlackBishop;
    const Piece rook   = (attacker == ChessColor::White) ? Piece::WhiteRook   : Piece::BlackRook;
    const Piece queen  = (attacker == ChessColor::White) ? Piece::WhiteQueen  : Piece::BlackQueen;
    const Piece king   = (attacker == ChessColor::White) ? Piece::WhiteKing   : Piece::BlackKing;

    const Bitboard occ = board.getAllOccupancy();
    Bitboard attacked = 0;

    Bitboard bb = board.getBitboard(pawn);
    while (bb)
    {
        const Square s = popLeastSignificantBit(bb);
        attacked |= (attacker == ChessColor::White)
            ? AttackTables::whitePawnAttacks(s)
            : AttackTables::blackPawnAttacks(s);
    }

    bb = board.getBitboard(knight);
    while (bb)
    {
        const Square s = popLeastSignificantBit(bb);
        attacked |= AttackTables::knightAttacks(s);
    }

    bb = board.getBitboard(king);
    while (bb)
    {
        const Square s = popLeastSignificantBit(bb);
        attacked |= AttackTables::kingAttacks(s);
    }

    bb = board.getBitboard(rook) | board.getBitboard(queen);
    while (bb)
    {
        const Square s = popLeastSignificantBit(bb);
        attacked |= AttackTables::rookAttacks(s, occ);
    }

    bb = board.getBitboard(bishop) | board.getBitboard(queen);
    while (bb)
    {
        const Square s = popLeastSignificantBit(bb);
        attacked |= AttackTables::bishopAttacks(s, occ);
    }

    return attacked;
}

// Helper function: returns ray bitboard between two squares (exclusive of both ends)
static Bitboard getBetweenRay(Square from, Square to)
{
    int fromIdx = static_cast<int>(from);
    int toIdx = static_cast<int>(to);
    int fromFile = fromIdx % 8;
    int fromRank = fromIdx / 8;
    int toFile = toIdx % 8;
    int toRank = toIdx / 8;

    int fileDiff = toFile - fromFile;
    int rankDiff = toRank - fromRank;

    int fileStep = 0;
    int rankStep = 0;

    if (fileDiff != 0) fileStep = (fileDiff > 0) ? 1 : -1;
    if (rankDiff != 0) rankStep = (rankDiff > 0) ? 1 : -1;

    // Validate it's a straight line or diagonal
    if (fileStep != 0 && rankStep != 0 && std::abs(fileDiff) != std::abs(rankDiff))
        return 0;
    if (fileStep == 0 && rankStep == 0)
        return 0;

    Bitboard ray = 0;
    int f = fromFile + fileStep;
    int r = fromRank + rankStep;

    while (f >= 0 && f < 8 && r >= 0 && r < 8)
    {
        if (f == toFile && r == toRank)
            break;
        ray |= (1ULL << (r * 8 + f));
        f += fileStep;
        r += rankStep;
    }

    return ray;
}

void MoveValidator::filterLegalMoves(
    Board& board,
    MoveList& moveList)
{
    MoveList legalMoves;

    for (int i = 0; i < moveList.size(); i++)
    {
        const Move& move = moveList[i];

        if (isMoveLegal(board, move))
        {
            legalMoves.add(move);
        }
    }

    moveList = legalMoves;
}

bool MoveValidator::isMoveLegal(
    Board& board,
    const Move& move)
{
    const Piece target = board.pieceAt(move.to);
    if (target == Piece::WhiteKing || target == Piece::BlackKing)
    {
        return false;
    }

    //--------------------------------------------------
    // Castling validation
    //--------------------------------------------------

    if (move.flag == MoveFlag::KingCastle)
    {
        ChessColor side = board.getSideToMove();

        if (isKingInCheck(board, side))
        {
            return false;
        }
        UndoInfo undo;
        Move stepMove(
            move.from,
            (side == ChessColor::White) ? Square::F1 : Square::F8,
            move.piece,
            MoveFlag::Quiet);

        board.makeMove(stepMove, undo);
        bool inCheckAfter = isKingInCheck(board, side);
        board.undoMove(stepMove, undo);

        if (inCheckAfter)
        {
            return false;
        }
    }
    else if (move.flag == MoveFlag::QueenCastle)
    {
        ChessColor side = board.getSideToMove();

        if (isKingInCheck(board, side))
        {
            return false;
        }
        UndoInfo undo;
        Move stepMove(
            move.from,
            (side == ChessColor::White) ? Square::D1 : Square::D8,
            move.piece,
            MoveFlag::Quiet);

        board.makeMove(stepMove, undo);
        bool inCheckAfter = isKingInCheck(board, side);
        board.undoMove(stepMove, undo);

        if (inCheckAfter)
        {
            return false;
        }
    }

    UndoInfo undoInfo;
    ChessColor side = board.getSideToMove();

    board.makeMove(move, undoInfo);
    bool legal = !isKingInCheck(board, side);
    board.undoMove(move, undoInfo);

    return legal;
}

bool MoveValidator::isSquareAttacked(
    const Board& board,
    Square square,
    ChessColor attacker)
{
    // Cache some bitboards to avoid repeated calls
    const Bitboard occ = board.getAllOccupancy();

    int sq = static_cast<int>(square);
    int file = sq % 8;
    int rank = sq / 8;

    // Pawn attacks (inverse lookup: which pawns attack this square)
    if (attacker == ChessColor::White) {
        // White pawns attack from one rank below
        if (rank > 0) {
            // Left diagonal attacker: from file+1 (source file < 7)
            if (file < 7 && board.pieceAt(static_cast<Square>(sq - 7)) == Piece::WhitePawn)
                return true;
            // Right diagonal attacker: from file-1 (source file > 0)
            if (file > 0 && board.pieceAt(static_cast<Square>(sq - 9)) == Piece::WhitePawn)
                return true;
        }
    } else {
        // Black pawns attack from one rank above
        if (rank < 7) {
            // Left diagonal attacker: from file+1 (source file < 7)
            if (file < 7 && board.pieceAt(static_cast<Square>(sq + 9)) == Piece::BlackPawn)
                return true;
            // Right diagonal attacker: from file-1 (source file > 0)
            if (file > 0 && board.pieceAt(static_cast<Square>(sq + 7)) == Piece::BlackPawn)
                return true;
        }
    }

    const Bitboard knightAttacks = AttackTables::knightAttacks(square);
    const Piece knight = (attacker == ChessColor::White) ? Piece::WhiteKnight : Piece::BlackKnight;
    if (knightAttacks & board.getBitboard(knight)) return true;

    const Bitboard kingAttacks = AttackTables::kingAttacks(square);
    const Piece king = (attacker == ChessColor::White) ? Piece::WhiteKing : Piece::BlackKing;
    if (kingAttacks & board.getBitboard(king)) return true;

    constexpr int RookDirections[4][2] =
    {
        { 0,  1},
        { 1,  0},
        { 0, -1},
        {-1,  0}
    };

    // Precompute attacker slider bitboards
    const Bitboard rookOrQueen = board.getBitboard((attacker == ChessColor::White) ? Piece::WhiteRook : Piece::BlackRook)
        | board.getBitboard((attacker == ChessColor::White) ? Piece::WhiteQueen : Piece::BlackQueen);

    for (int dir = 0; dir < 4; dir++)
    {
        int file = static_cast<int>(square) % 8;
        int rank = static_cast<int>(square) / 8;

        while (true)
        {
            file += RookDirections[dir][0];
            rank += RookDirections[dir][1];

            if (file < 0 || file >= 8 || rank < 0 || rank >= 8)
            {
                break;
            }

            const Square target = static_cast<Square>(rank * 8 + file);

            if (!getBit(occ, target))
            {
                continue;
            }

            if (getBit(rookOrQueen, target))
            {
                return true;
            }

            break;
        }
    }

    constexpr int BishopDirections[4][2] =
    {
        { 1,  1},
        { 1, -1},
        {-1, -1},
        {-1,  1}
    };

    const Bitboard bishopOrQueen = board.getBitboard((attacker == ChessColor::White) ? Piece::WhiteBishop : Piece::BlackBishop)
        | board.getBitboard((attacker == ChessColor::White) ? Piece::WhiteQueen : Piece::BlackQueen);

    for (int dir = 0; dir < 4; dir++)
    {
        int file = static_cast<int>(square) % 8;
        int rank = static_cast<int>(square) / 8;

        while (true)
        {
            file += BishopDirections[dir][0];
            rank += BishopDirections[dir][1];

            if (file < 0 || file >= 8 || rank < 0 || rank >= 8)
            {
                break;
            }

            const Square target = static_cast<Square>(rank * 8 + file);

            if (!getBit(occ, target))
            {
                continue;
            }

            if (getBit(bishopOrQueen, target))
            {
                return true;
            }

            break;
        }
    }

    return false;
}

bool MoveValidator::isKingInCheck(
    const Board& board,
    ChessColor side)
{
    Square kingSquare =
        findKing(board, side);

    if (kingSquare == Square::None)
    {
        return false;
    }

    bool inCheck = isSquareAttacked(
        board,
        kingSquare,
        oppositeColor(side));

    return inCheck;
}

ChessColor MoveValidator::oppositeColor(ChessColor color)
{
    return (color == ChessColor::White)
        ? ChessColor::Black
        : ChessColor::White;
}

Square MoveValidator::findKing(
    const Board& board,
    ChessColor side)
{
    Piece king =
        (side == ChessColor::White)
        ? Piece::WhiteKing
        : Piece::BlackKing;

    Bitboard kings = board.getBitboard(king);

    if (kings == 0)
    {
        return Square::None;
    }

    return popLeastSignificantBit(kings);
}

int MoveValidator::pieceValue(Piece piece)
{
    switch (piece)
    {
    case Piece::WhitePawn:   case Piece::BlackPawn:   return 1000;
    case Piece::WhiteKnight: case Piece::BlackKnight: return 3200;
    case Piece::WhiteBishop: case Piece::BlackBishop: return 3300;
    case Piece::WhiteRook:   case Piece::BlackRook:   return 5000;
    case Piece::WhiteQueen:  case Piece::BlackQueen:  return 9000;
    case Piece::WhiteKing:   case Piece::BlackKing:   return 200000;
    default: return 0;
    }
}

// Zwraca najtańszą figurę danego koloru, która atakuje pole.
// Sprawdza kolejno: pionki, skoczki, gońce, wieże, hetmany, króla.
// Zapisuje pole atakującego w `fromSquare`.
// Ważne: dla gońców/wież/hetmanów sprawdzamy KONKRETNĄ figurę
// (czy promień od tej figury do pola jest czysty), a nie isSquareAttacked
// (które zwraca true, gdy JAKAKOLWIEK figura tego koloru atakuje pole).
Piece MoveValidator::leastValuableAttacker(
    const Board& board,
    Square square,
    ChessColor attacker,
    Square& fromSquare)
{
    const auto pawn = (attacker == ChessColor::White) ? Piece::WhitePawn : Piece::BlackPawn;
    const auto knight = (attacker == ChessColor::White) ? Piece::WhiteKnight : Piece::BlackKnight;
    const auto bishop = (attacker == ChessColor::White) ? Piece::WhiteBishop : Piece::BlackBishop;
    const auto rook = (attacker == ChessColor::White) ? Piece::WhiteRook : Piece::BlackRook;
    const auto queen = (attacker == ChessColor::White) ? Piece::WhiteQueen : Piece::BlackQueen;
    const auto king = (attacker == ChessColor::White) ? Piece::WhiteKing : Piece::BlackKing;

    const int targetIndex = static_cast<int>(square);
    const int targetFile = targetIndex % 8;
    const int targetRank = targetIndex / 8;

    // Pionki
    Bitboard pawns = board.getBitboard(pawn);
    while (pawns)
    {
        Square from = popLeastSignificantBit(pawns);
        const Bitboard attacks = (attacker == ChessColor::White)
            ? AttackTables::whitePawnAttacks(from)
            : AttackTables::blackPawnAttacks(from);
        if (getBit(attacks, square))
        {
            fromSquare = from;
            return pawn;
        }
    }

    // Skoczkowie
    Bitboard knights = board.getBitboard(knight);
    while (knights)
    {
        Square from = popLeastSignificantBit(knights);
        if (getBit(AttackTables::knightAttacks(from), square))
        {
            fromSquare = from;
            return knight;
        }
    }

    // Gońce - sprawdzamy każdą konkretną figurę po przekątnych
    Bitboard bishops = board.getBitboard(bishop);
    while (bishops)
    {
        Square from = popLeastSignificantBit(bishops);
        if (isDiagonalAttacker(board, from, targetFile, targetRank))
        {
            fromSquare = from;
            return bishop;
        }
    }

    // Wieże - sprawdzamy każdą konkretną figurę po liniach prostych
    Bitboard rooks = board.getBitboard(rook);
    while (rooks)
    {
        Square from = popLeastSignificantBit(rooks);
        if (isStraightAttacker(board, from, targetFile, targetRank))
        {
            fromSquare = from;
            return rook;
        }
    }

    // Hetmany - atakują i po przekątnych, i po liniach prostych
    Bitboard queens = board.getBitboard(queen);
    while (queens)
    {
        Square from = popLeastSignificantBit(queens);
        if (isDiagonalAttacker(board, from, targetFile, targetRank) ||
            isStraightAttacker(board, from, targetFile, targetRank))
        {
            fromSquare = from;
            return queen;
        }
    }

    // Król
    Bitboard kings = board.getBitboard(king);
    while (kings)
    {
        Square from = popLeastSignificantBit(kings);
        if (getBit(AttackTables::kingAttacks(from), square))
        {
            fromSquare = from;
            return king;
        }
    }

    fromSquare = Square::None;
    return Piece::None;
}

// Sprawdza, czy figura na polu `from` atakuje pole (targetFile, targetRank)
// po linii prostej (wiersz/kolumna) bez żadnych blokerów.
bool MoveValidator::isStraightAttacker(
    const Board& board,
    Square from,
    int targetFile,
    int targetRank)
{
    const int fromIndex = static_cast<int>(from);
    const int fromFile = fromIndex % 8;
    const int fromRank = fromIndex / 8;

    if (fromFile != targetFile && fromRank != targetRank)
    {
        return false; // nie ta sama linia/kolumna
    }

    const int fileStep = (fromFile == targetFile) ? 0 : ((targetFile > fromFile) ? 1 : -1);
    const int rankStep = (fromRank == targetRank) ? 0 : ((targetRank > fromRank) ? 1 : -1);

    int file = fromFile + fileStep;
    int rank = fromRank + rankStep;

    while (file != targetFile || rank != targetRank)
    {
        if (board.pieceAt(static_cast<Square>(rank * 8 + file)) != Piece::None)
        {
            return false; // bloker
        }

        file += fileStep;
        rank += rankStep;
    }

    return true;
}

// Sprawdza, czy figura na polu `from` atakuje pole (targetFile, targetRank)
// po przekątnej bez żadnych blokerów.
bool MoveValidator::isDiagonalAttacker(
    const Board& board,
    Square from,
    int targetFile,
    int targetRank)
{
    const int fromIndex = static_cast<int>(from);
    const int fromFile = fromIndex % 8;
    const int fromRank = fromIndex / 8;

    const int fileDiff = targetFile - fromFile;
    const int rankDiff = targetRank - fromRank;

    if (std::abs(fileDiff) != std::abs(rankDiff) || fileDiff == 0)
    {
        return false; // nie na przekątnej
    }

    const int fileStep = (fileDiff > 0) ? 1 : -1;
    const int rankStep = (rankDiff > 0) ? 1 : -1;

    int file = fromFile + fileStep;
    int rank = fromRank + rankStep;

    while (file != targetFile || rank != targetRank)
    {
        if (board.pieceAt(static_cast<Square>(rank * 8 + file)) != Piece::None)
        {
            return false; // bloker
        }

        file += fileStep;
        rank += rankStep;
    }

    return true;
}

// Rekurencyjna część SEE. `value` to wartość figury, która właśnie
// została "postawiona" na polu (i którą może zbić strona do ruchu).
// Zwraca bilans wymiany z perspektywy strony, która ZACZĘŁA atak.
int MoveValidator::seeRecursive(
    Board& board,
    Square square,
    ChessColor sideToMove,
    int value)
{
    Square fromSquare;
    Piece attacker = leastValuableAttacker(board, square, sideToMove, fromSquare);

    if (attacker == Piece::None)
    {
        // Nikt nie może zbić - strona, która ma teraz bić, nie może
        // nic zyskać, więc rezygnuje. Bilans to 0 (nie bije).
        return 0;
    }

    // Usuń atakującego z planszy (symulacja jego zbicia)
    board.removePiece(attacker, fromSquare);

    // Rekurencyjnie: strona przeciwna może teraz odbić.
    // Od wyniku "odbicia" odejmujemy wartość figury, którą właśnie
    // zbitiśmy (bo to zysk strony, która zaczęła).
    const int opponentGain = seeRecursive(
        board,
        square,
        oppositeColor(sideToMove),
        pieceValue(attacker));

// Bilans z perspektywy zaczynającej atak:
    //   gain = wartość zbitej figury (value) - najlepszy wynik przeciwnika
    int gain = value - opponentGain;

    // Zwracamy wynik ze znakiem (zgodnie z komentarzem w nagłówku):
    // dodatni = korzystna wymiana, ujemny = stratna.
    return gain;
}

// Static Exchange Evaluation (SEE): symuluje wymianę na polu.
// Zwraca bilans materiałowy z perspektywy strony, która zaczyna atak.
// Ujemny wynik = strona zaczynająca traci; dodatni = zyskuje.
int MoveValidator::see(
    const Board& board,
    Square square)
{
    const Piece victim = board.pieceAt(square);
    if (victim == Piece::None)
    {
        return 0;
    }

// Strona, która zaczyna atak, to przeciwnik koloru figury na polu.
    ChessColor attackerToMove = oppositeColor(getPieceColor(victim));

    Board copy = board;

    // Usuń bity kawałek (victim) z kopii ZANIM rozpoczniemy rekurencję SEE.
    // Bez tego figura broniona tylko przez samą siebie (tj. wisząca) byłaby
    // błędnie uznawana za chronioną – `leastValuableAttacker` mógłby "zbić"
    // tą samą figurę, która jest ofiarą, i zaniżać stratę (gubienie hetmana).
    copy.removePiece(victim, square);

    return seeRecursive(
        copy,
        square,
        attackerToMove,
        pieceValue(victim));
}

namespace
{
Bitboard forkAttacks(const Board& board, Piece piece, Square square)
{
    const Bitboard occupancy = board.getAllOccupancy();
    switch (piece)
    {
    case Piece::WhiteKnight:
    case Piece::BlackKnight:
        return AttackTables::knightAttacks(square);
    case Piece::WhiteBishop:
    case Piece::BlackBishop:
        return AttackTables::bishopAttacks(square, occupancy);
    case Piece::WhiteRook:
    case Piece::BlackRook:
        return AttackTables::rookAttacks(square, occupancy);
    case Piece::WhiteQueen:
    case Piece::BlackQueen:
        return AttackTables::queenAttacks(square, occupancy);
    default:
        return 0;
    }
}

int pieceValueMP(Piece piece)
{
    switch (piece)
    {
    case Piece::WhitePawn: case Piece::BlackPawn: return 1000;
    case Piece::WhiteKnight: case Piece::BlackKnight: return 3200;
    case Piece::WhiteBishop: case Piece::BlackBishop: return 3300;
    case Piece::WhiteRook: case Piece::BlackRook: return 5000;
    case Piece::WhiteQueen: case Piece::BlackQueen: return 9000;
    default: return 0;
    }
}

int forkPenalty(const Board& board, ChessColor attackerColor)
{
    const ChessColor victimColor = MoveValidator::oppositeColor(attackerColor);
    const Bitboard attackers = board.getOccupancy(attackerColor);
    Bitboard pieces = attackers;
    int totalPenalty = 0;

    while (pieces)
    {
        const Square square = popLeastSignificantBit(pieces);
        const Piece attacker = board.pieceAt(square);
        if (attacker == Piece::WhitePawn || attacker == Piece::BlackPawn ||
            attacker == Piece::WhiteKing || attacker == Piece::BlackKing)
        {
            continue;
        }

        Bitboard targets = forkAttacks(board, attacker, square) & board.getOccupancy(victimColor);
        int count = 0;
        int smallestVictim = 200000;
        while (targets)
        {
            const Square target = popLeastSignificantBit(targets);
            const Piece victim = board.pieceAt(target);
            if (victim != Piece::WhiteKing && victim != Piece::BlackKing &&
                !MoveValidator::isSquareAttacked(board, target, victimColor))
            {
                ++count;
                smallestVictim = std::min(smallestVictim, pieceValueMP(victim));
            }
        }

        if (count >= 2)
        {
            totalPenalty += std::min(1000, smallestVictim / 2);
        }
    }
    return totalPenalty;
}
}

int MoveValidator::evaluateTactics(
    const Board& board)
{
    // Pojedyncze przejście po wszystkich polach. Zwraca wynik netto
    // (biały - czarny): kara za "wiszące" figury strony posiadającej
    // daną figurę. Dzięki temu ewaluacja wywołuje tę funkcję tylko
    // raz zamiast osobno dla każdego koloru.
    int score = 0;

    for (int index = 0; index < 64; ++index)
    {
        const Square square = static_cast<Square>(index);
        const Piece piece = board.pieceAt(square);

        if (piece == Piece::None)
        {
            continue;
        }

        // Króla nie oceniamy - nie ma sensu karać za atak na króla
        if (piece == Piece::WhiteKing || piece == Piece::BlackKing)
        {
            continue;
        }

        const ChessColor color = getPieceColor(piece);
        const ChessColor enemy =
            (color == ChessColor::White)
                ? ChessColor::Black
                : ChessColor::White;

        int penalty = 0;

        // Czy figura jest atakowana przez przeciwnika?
        if (isSquareAttacked(board, square, enemy))
        {
            const int value = pieceValue(piece);

            // SEE: zwraca bilans wymiany z perspektywy ATAKUJĄCEGO (przeciwnika).
            // seeScore > 0  => przeciwnik zyskuje na wymianie => MY tracimy.
            // seeScore == 0 => przeciwnik nie zyskuje (może zrezygnować) => bezpieczne.
            const int seeScore = see(board, square);

            if (seeScore > 0)
            {
                // Przeciwnik zyskuje materiał na wymianie - faktycznie
                // "wisząca" figura. Kara za stratę.
                penalty = std::min(seeScore, value);
            }
            else
            {
                // Brak opłacalnego bicia - figura w praktyce nie jest
                // stracona. Minimalna, stała kara za bycie atakowanym
                // (deformacja pozycyjna), by nie karcić legalnych ruchów
                // na atakowane pola.
                penalty = 100;
            }
        }

        // Kara obciąża stronę, której figura należy.
        if (color == ChessColor::White)
        {
            score -= penalty;
        }
        else
        {
            score += penalty;
        }
    }

    // A fork is a threat to the attacked side, not to the forking piece.
    score += forkPenalty(board, ChessColor::White);
    score -= forkPenalty(board, ChessColor::Black);

    return score;
}

void MoveValidator::updatePieceBitboards(
    Board& board,
    const MoveList& legalMoves)
{
    board.clearAllPieceBitboards();

    for (int i = 0; i < legalMoves.size(); ++i)
    {
        const Move& move = legalMoves[i];
        Bitboard moveBit = 0;
        setBit(moveBit, move.to);
        board.setPieceMoves(move.from, moveBit);

        if (move.flag == MoveFlag::KingCastle)
        {
            if (move.piece == Piece::WhiteKing)
            {
                Bitboard rookMove = 0;
                setBit(rookMove, Square::F1);
                board.setPieceMoves(Square::H1, rookMove);
            }
            else
            {
                Bitboard rookMove = 0;
                setBit(rookMove, Square::F8);
                board.setPieceMoves(Square::H8, rookMove);
            }
        }
        else if (move.flag == MoveFlag::QueenCastle)
        {
            if (move.piece == Piece::WhiteKing)
            {
                Bitboard rookMove = 0;
                setBit(rookMove, Square::D1);
                board.setPieceMoves(Square::A1, rookMove);
            }
            else
            {
                Bitboard rookMove = 0;
                setBit(rookMove, Square::D8);
                board.setPieceMoves(Square::A8, rookMove);
            }
        }
    }

    const Bitboard occ = board.getAllOccupancy();

    for (int index = 0; index < 64; ++index)
    {
        const Square square = static_cast<Square>(index);
        const Piece piece = board.pieceAt(square);

        if (piece == Piece::None)
        {
            continue;
        }

        Bitboard attacks = 0;

        switch (piece)
        {
        case Piece::WhitePawn:
            attacks = AttackTables::whitePawnAttacks(square);
            break;

        case Piece::BlackPawn:
            attacks = AttackTables::blackPawnAttacks(square);
            break;

        case Piece::WhiteKnight:
        case Piece::BlackKnight:
            attacks = AttackTables::knightAttacks(square);
            break;

        case Piece::WhiteKing:
        case Piece::BlackKing:
            attacks = AttackTables::kingAttacks(square);
            break;

        case Piece::WhiteBishop:
        case Piece::BlackBishop:
            attacks = AttackTables::bishopAttacks(square, occ);
            break;

        case Piece::WhiteRook:
        case Piece::BlackRook:
            attacks = AttackTables::rookAttacks(square, occ);
            break;

        case Piece::WhiteQueen:
        case Piece::BlackQueen:
            attacks = AttackTables::queenAttacks(square, occ);
            break;

        default:
            break;
        }

        board.setPieceAttacks(square, attacks);
    }
}


MoveValidator::CheckInfo MoveValidator::computeCheckInfo(const Board& board, ChessColor side)
{
    CheckInfo info;
    info.kingSquare = findKing(board, side);

    if (info.kingSquare == Square::None)
        return info;

    ChessColor enemy = oppositeColor(side);

    // Compute all enemy attacks once
    info.enemyAttacks = computeAttackBoardFromBoard(board, enemy);

    // Check if king is in check
    info.inCheck = getBit(info.enemyAttacks, info.kingSquare);

    // These are needed for pin detection regardless of check status
    int kingIdx = static_cast<int>(info.kingSquare);
    int kingFile = kingIdx % 8;
    int kingRank = kingIdx / 8;

    const Bitboard enemyRooks = board.getBitboard(
        enemy == ChessColor::White ? Piece::WhiteRook : Piece::BlackRook);
    const Bitboard enemyBishops = board.getBitboard(
        enemy == ChessColor::White ? Piece::WhiteBishop : Piece::BlackBishop);
    const Bitboard enemyQueens = board.getBitboard(
        enemy == ChessColor::White ? Piece::WhiteQueen : Piece::BlackQueen);
    const Bitboard enemyOrthogonal = enemyRooks | enemyQueens;
    const Bitboard enemyDiagonal = enemyBishops | enemyQueens;

    // Find all checkers (only if in check)
    if (info.inCheck)
    {
        Bitboard enemyPawns   = board.getBitboard(enemy == ChessColor::White ? Piece::WhitePawn   : Piece::BlackPawn);
        Bitboard enemyKnights = board.getBitboard(enemy == ChessColor::White ? Piece::WhiteKnight : Piece::BlackKnight);
        Bitboard enemyKing    = board.getBitboard(enemy == ChessColor::White ? Piece::WhiteKing   : Piece::BlackKing);

        // Pawn checks
        Bitboard pawnAttacks = (enemy == ChessColor::White)
            ? AttackTables::blackPawnAttacks(info.kingSquare)
            : AttackTables::whitePawnAttacks(info.kingSquare);
        info.checkers |= (pawnAttacks & enemyPawns);

        // Knight checks
        Bitboard knightAttacks = AttackTables::knightAttacks(info.kingSquare);
        info.checkers |= (knightAttacks & enemyKnights);

        // King checks (adjacent)
        Bitboard kingAttacks = AttackTables::kingAttacks(info.kingSquare);
        info.checkers |= (kingAttacks & enemyKing);

        // Slider checks (bishop/rook/queen) - ray from king outward
        constexpr int RookDirs[4][2]   = {{1,0},{-1,0},{0,1},{0,-1}};
        constexpr int BishopDirs[4][2] = {{1,1},{1,-1},{-1,1},{-1,-1}};

        // Orthogonal rays
        for (int d = 0; d < 4; ++d)
        {
            int f = kingFile + RookDirs[d][0];
            int r = kingRank + RookDirs[d][1];
            while (f >= 0 && f < 8 && r >= 0 && r < 8)
            {
                Square sq = static_cast<Square>(r * 8 + f);
                Piece piece = board.pieceAt(sq);
                if (piece != Piece::None)
                {
                    if (getBit(enemyOrthogonal, sq))
                        info.checkers |= (1ULL << static_cast<int>(sq));
                    break;
                }
                f += RookDirs[d][0];
                r += RookDirs[d][1];
            }
        }

        // Diagonal rays
        for (int d = 0; d < 4; ++d)
        {
            int f = kingFile + BishopDirs[d][0];
            int r = kingRank + BishopDirs[d][1];
            while (f >= 0 && f < 8 && r >= 0 && r < 8)
            {
                Square sq = static_cast<Square>(r * 8 + f);
                Piece piece = board.pieceAt(sq);
                if (piece != Piece::None)
                {
                    if (getBit(enemyDiagonal, sq))
                        info.checkers |= (1ULL << static_cast<int>(sq));
                    break;
                }
                f += BishopDirs[d][0];
                r += BishopDirs[d][1];
            }
        }

        info.doubleCheck = (countBits(info.checkers) >= 2);
    }

    // Find pinned pieces (our sliders on same ray as king with enemy slider behind)
    Bitboard ownBishops = board.getBitboard(side == ChessColor::White ? Piece::WhiteBishop : Piece::BlackBishop);
    Bitboard ownRooks   = board.getBitboard(side == ChessColor::White ? Piece::WhiteRook   : Piece::BlackRook);
    Bitboard ownQueens  = board.getBitboard(side == ChessColor::White ? Piece::WhiteQueen  : Piece::BlackQueen);
    Bitboard ownPawns   = board.getBitboard(side == ChessColor::White ? Piece::WhitePawn   : Piece::BlackPawn);
    Bitboard ownSliders = ownBishops | ownRooks | ownQueens;
    Bitboard ownPawnsForPins = ownPawns;

    // For each own slider (bishop, rook, queen), check if it's pinned
    Bitboard sliders = ownSliders;
    while (sliders)
    {
        Square sliderSq = popLeastSignificantBit(sliders);
        Piece sliderPiece = board.pieceAt(sliderSq);

        bool isBishop = (sliderPiece == Piece::WhiteBishop || sliderPiece == Piece::BlackBishop);
        bool isRook   = (sliderPiece == Piece::WhiteRook   || sliderPiece == Piece::BlackRook);
        bool isQueen  = (sliderPiece == Piece::WhiteQueen  || sliderPiece == Piece::BlackQueen);

        // Check each direction from king through this slider
        constexpr int AllDirs[8][2] = {
            {1,0},{-1,0},{0,1},{0,-1},  // orthogonal
            {1,1},{1,-1},{-1,1},{-1,-1} // diagonal
        };

        for (int d = 0; d < 8; ++d)
        {
            int f = kingFile + AllDirs[d][0];
            int r = kingRank + AllDirs[d][1];
            bool foundOwnSlider = false;

            while (f >= 0 && f < 8 && r >= 0 && r < 8)
            {
                Square sq = static_cast<Square>(r * 8 + f);
                Piece piece = board.pieceAt(sq);

                if (piece != Piece::None)
                {
                    if (sq == sliderSq)
                    {
                        foundOwnSlider = true;
                    }
                    else
                    {
                        // Found a blocker before or after our slider
                        if (foundOwnSlider)
                        {
                            // Check if blocker is enemy slider of correct type
                            bool isOrtho = (d < 4);
                            bool isDiag  = (d >= 4);

                            if ((isOrtho && (isRook || isQueen) && getBit(enemyOrthogonal, sq)) ||
                                (isDiag && (isBishop || isQueen) && getBit(enemyDiagonal, sq)))
                            {
                                // This piece is pinned!
                                info.pinned |= (1ULL << static_cast<int>(sliderSq));

                                // Pin ray: squares between slider and king (inclusive) + squares beyond slider towards enemy
                                Bitboard ray = getBetweenRay(info.kingSquare, sliderSq) | (1ULL << static_cast<int>(sliderSq));

                                // Extend ray beyond slider towards enemy
                                int ef = f + AllDirs[d][0];
                                int er = r + AllDirs[d][1];
                                while (ef >= 0 && ef < 8 && er >= 0 && er < 8)
                                {
                                    ray |= (1ULL << (er * 8 + ef));
                                    ef += AllDirs[d][0];
                                    er += AllDirs[d][1];
                                }

                                info.pinRays[static_cast<int>(sliderSq)] = ray;
                            }
                        }
                        break;
                    }
                }
                f += AllDirs[d][0];
                r += AllDirs[d][1];
            }
        }
    }

    // Check for pinned pawns (orthogonal rays only - same file as king with enemy rook/queen behind)
    Bitboard pawns = ownPawnsForPins;
    while (pawns)
    {
        Square pawnSq = popLeastSignificantBit(pawns);
        int pawnIdx = static_cast<int>(pawnSq);
        int pawnFile = pawnIdx % 8;
        int pawnRank = pawnIdx / 8;

        // Check if pawn is on same file as king
        if (pawnFile == kingFile)
        {
            // Determine direction from king to pawn
            int rankDiff = pawnRank - kingRank;
            int direction = (rankDiff > 0) ? 1 : -1;

            // Look for enemy rook/queen behind the pawn (further along same file)
            int r = pawnRank + direction;
            while (r >= 0 && r < 8)
            {
                Square sq = static_cast<Square>(r * 8 + kingFile);
                Piece piece = board.pieceAt(sq);
                if (piece != Piece::None)
                {
                    if (getBit(enemyOrthogonal, sq))
                    {
                        // Pawn is pinned by enemy rook/queen on same file
                        info.pinned |= (1ULL << static_cast<int>(pawnSq));

                        // Pin ray: squares between pawn and king (inclusive) + squares beyond pawn towards enemy
                        Bitboard ray = getBetweenRay(info.kingSquare, pawnSq) | (1ULL << static_cast<int>(pawnSq));

                        // Extend ray beyond pawn towards enemy
                        int r2 = pawnRank + direction;
                        while (r2 >= 0 && r2 < 8)
                        {
                            ray |= (1ULL << (r2 * 8 + kingFile));
                            r2 += direction;
                        }

                        info.pinRays[static_cast<int>(pawnSq)] = ray;
                    }
                    break;
                }
                r += direction;
            }
        }
    }

    return info;
}

