#include "MoveGenerator.h"

#include "AttackTables.h"
#include "Bitboard.h"
#include "Move.h"
#include "MoveValidator.h"
#include "Foundation/Bitboards.h"

namespace
{
inline int sideIndex(ChessColor c) { return c == ChessColor::White ? 0 : 1; }
}

// Lokalny pomocnik: promień między dwoma polami.
static Bitboard getBetweenRay(Square from, Square to);

void MoveGenerator::generateMoves(
    const Board& board,
    MoveList& moveList,
    const MoveValidator::CheckInfo& checkInfo)
{
    // Bitboardy biorą wyłącznie dane już wygenerowane przez silnik
    // (Board + AttackTables); bez kosztownego liczenia szachów/przypięć.
    const Bitboards bitboards = Bitboards::compute(board, false);
    generateMoves(board, bitboards, moveList, checkInfo);
}

void MoveGenerator::generateCaptures(
    const Board& board,
    MoveList& moveList,
    const MoveValidator::CheckInfo& checkInfo)
{
    const Bitboards bitboards = Bitboards::compute(board, false);
    generateCaptures(board, bitboards, moveList, checkInfo);
}

void MoveGenerator::generateMoves(
    const Board& board,
    const Bitboards& bitboards,
    MoveList& moveList,
    const MoveValidator::CheckInfo& checkInfo)
{
    moveList.clear();

    // Generowanie ruchów musi być kompletne. Obecna optymalizacja oparta na
    // CheckInfo nie obejmuje wszystkich przypadków związania i odpowiedzi na
    // szacha; legalność jest dlatego sprawdzana przez MoveValidator po
    // wygenerowaniu listy. Używamy tu zerowej maski, aby nie odrzucać
    // prawidłowych pseudoruchów przed tą walidacją.
    (void)checkInfo;
    const MoveValidator::CheckInfo pseudoInfo{};

    // KRÓL wymaga prawdziwego CheckInfo: bez niego generator nie zna pola
    // atakowanego przez wroga i statusu szacha, przez co generuje roszady
    // w szachu oraz przez pole atakowane (odrzucane tylko przy roszadzie —
    // po zwykłym ruchu króla filtr isKingInCheck nadal ratuje sprawę).
    const MoveValidator::CheckInfo kingInfo =
        MoveValidator::computeCheckInfo(board, board.getSideToMove());

    generateKingMoves(board, bitboards, moveList, kingInfo);

    generateQueenMoves(board, bitboards, moveList, pseudoInfo);

    generateRookMoves(board, bitboards, moveList, pseudoInfo);

    generateBishopMoves(board, bitboards, moveList, pseudoInfo);

    generateKnightMoves(board, bitboards, moveList, pseudoInfo);

    generatePawnMoves(board, bitboards, moveList, pseudoInfo);
}

void MoveGenerator::generateCaptures(
    const Board& board,
    const Bitboards& bitboards,
    MoveList& moveList,
    const MoveValidator::CheckInfo& checkInfo)
{
    moveList.clear();

    // Zobacz komentarz w generateMoves: kompletność pseudoruchów jest
    // ważniejsza niż przedwczesne ograniczanie ich niepełnym CheckInfo.
    (void)checkInfo;
    const MoveValidator::CheckInfo pseudoInfo{};

    generateQueenCaptures(board, bitboards, moveList, pseudoInfo);

    generateRookCaptures(board, bitboards, moveList, pseudoInfo);

    generateBishopCaptures(board, bitboards, moveList, pseudoInfo);

    generateKnightCaptures(board, bitboards, moveList, pseudoInfo);

    generatePawnCaptures(board, bitboards, moveList, pseudoInfo);

    generateKingCaptures(board, bitboards, moveList, pseudoInfo);
}

// Helper functions for generating moves with CheckInfo

void MoveGenerator::generatePawnMoves(
    const Board& board,
    const Bitboards& bitboards,
    MoveList& moveList,
    const MoveValidator::CheckInfo& checkInfo)
{
    ChessColor side = board.getSideToMove();

    Piece pawn =
        (side == ChessColor::White)
        ? Piece::WhitePawn
        : Piece::BlackPawn;

    Bitboard pawns = bitboards.pawns[sideIndex(side)];

    while (pawns)
    {
        Square from = popLeastSignificantBit(pawns);

        int file = static_cast<int>(from) % 8;
        int rank = static_cast<int>(from) / 8;

        //-------------------------------------------------
        // One-square push
        //-------------------------------------------------

        int forwardRank =
            rank + (side == ChessColor::White ? 1 : -1);

        if (forwardRank >= 0 && forwardRank < 8)
        {
            Square to =
                static_cast<Square>(forwardRank * 8 + file);

            if (!getBit(bitboards.allOccupied, to))
            {
                // Check if this square is allowed by pin/eviction constraints
                bool squareAllowed = true;
                if (checkInfo.pinned & (1ULL << static_cast<int>(from)))
                {
                    // Piece is pinned, check if move is along pin ray
                    if (!(checkInfo.pinRays[static_cast<int>(from)] & (1ULL << static_cast<int>(to))))
                    {
                        squareAllowed = false;
                    }
                }
                else if (checkInfo.inCheck)
                {
                    // In check, check if this is an evasion move
                    if (!(checkInfo.enemyAttacks & (1ULL << static_cast<int>(to))) &&
                        !(checkInfo.checkers & (1ULL << static_cast<int>(to))))
                    {
                        // Not a capture of checker or block - only king moves allowed in double check
                        if (checkInfo.doubleCheck)
                        {
                            squareAllowed = false;
                        }
                        // For single check, non-capture non-block moves are not allowed
                        // unless it's a king move (handled separately)
                        if (!checkInfo.doubleCheck)
                        {
                            // Check if it's a block move
                            bool isBlock = false;
                            Bitboard checkers = checkInfo.checkers;
                            while (checkers)
                            {
                                Square checkerSq = popLeastSignificantBit(checkers);
                                if (getBit(getBetweenRay(from, to), checkerSq))
                                {
                                    isBlock = true;
                                    break;
                                }
                            }
                            if (!isBlock)
                            {
                                squareAllowed = false;
                            }
                        }
                    }
                    // If it's a capture, check if it captures a checker
                    else if (board.pieceAt(to) != Piece::None &&
                            getPieceColor(board.pieceAt(to)) != side)
                    {
                        // Capture - only allowed if it captures a checker
                        if (!(checkInfo.checkers & (1ULL << static_cast<int>(to))))
                        {
                            squareAllowed = false;
                        }
                    }
                }

                if (squareAllowed)
                {
                    bool promotion =
                        (side == ChessColor::White && forwardRank == 7) ||
                        (side == ChessColor::Black && forwardRank == 0);

                    if (promotion)
                    {
                        moveList.add(
                            Move(from, to, pawn, MoveFlag::PromotionKnight));

                        moveList.add(
                            Move(from, to, pawn, MoveFlag::PromotionBishop));

                        moveList.add(
                            Move(from, to, pawn, MoveFlag::PromotionRook));

                        moveList.add(
                            Move(from, to, pawn, MoveFlag::PromotionQueen));
                    }
                    else
                    {
                        moveList.add(
                            Move(from, to, pawn, MoveFlag::Quiet));

                        //-----------------------------------------
                        // Double push
                        //-----------------------------------------

                        bool startRank =
                            (side == ChessColor::White && rank == 1) ||
                            (side == ChessColor::Black && rank == 6);

                        if (startRank)
                        {
                            int secondRank =
                                rank + (side == ChessColor::White ? 2 : -2);

                            Square second =
                                static_cast<Square>(secondRank * 8 + file);

                            if (!getBit(bitboards.allOccupied, second))
                            {
                                // Check if double push square is allowed
                                bool doublePushAllowed = true;
                                if (checkInfo.pinned & (1ULL << static_cast<int>(from)))
                                {
                                    // Piece is pinned, check if move is along pin ray
                                    if (!(checkInfo.pinRays[static_cast<int>(from)] & (1ULL << static_cast<int>(second))))
                                    {
                                        doublePushAllowed = false;
                                    }
                                }
                                else if (checkInfo.inCheck)
                                {
                                    // In check, double push only allowed if it's an evasion
                                    // (capture of checker or block)
                                    bool isEvasion = false;

                                    // Check if it captures a checker (impossible for double push to same file)
                                    // Check if it's a block
                                    Bitboard checkers = checkInfo.checkers;
                                    while (checkers)
                                    {
                                        Square checkerSq = popLeastSignificantBit(checkers);
                                        if (getBit(getBetweenRay(from, second), checkerSq))
                                        {
                                            isEvasion = true;
                                            break;
                                        }
                                    }

                                    if (!isEvasion)
                                    {
                                        doublePushAllowed = false;
                                    }
                                }

                                if (doublePushAllowed)
                                {
                                    moveList.add(
                                        Move(from, second, pawn, MoveFlag::DoublePawnPush));
                                }
                            }
                        }
                    }
                }
            }
        }

        //-------------------------------------------------
        // Captures
        //-------------------------------------------------

        Bitboard attacks =
            (side == ChessColor::White)
            ? AttackTables::whitePawnAttacks(from)
            : AttackTables::blackPawnAttacks(from);

        while (attacks)
        {
            Square to =
                popLeastSignificantBit(attacks);

            Piece target =
                board.pieceAt(to);

            if (target == Piece::None)
                continue;

            if (getPieceColor(target) == side)
                continue;

            // For captures, check if allowed by pin/eviction constraints
            bool captureAllowed = true;
            if (checkInfo.pinned & (1ULL << static_cast<int>(from)))
            {
                // Piece is pinned, check if capture is along pin ray
                if (!(checkInfo.pinRays[static_cast<int>(from)] & (1ULL << static_cast<int>(to))))
                {
                    captureAllowed = false;
                }
            }
            else if (checkInfo.inCheck)
            {
                // In check, only captures of checkers are allowed (or blocks, but pawns can't block with capture)
                if (!(checkInfo.checkers & (1ULL << static_cast<int>(to))))
                {
                    captureAllowed = false;
                }
            }

            if (captureAllowed)
            {
                bool promotion =
                    (side == ChessColor::White && rank == 6) ||
                    (side == ChessColor::Black && rank == 1);

                if (promotion)
                {
                    moveList.add(
                        Move(from, to, pawn,
                            MoveFlag::PromotionCaptureKnight, target));

                    moveList.add(
                        Move(from, to, pawn,
                            MoveFlag::PromotionCaptureBishop, target));

                    moveList.add(
                        Move(from, to, pawn,
                            MoveFlag::PromotionCaptureRook, target));

                    moveList.add(
                        Move(from, to, pawn,
                            MoveFlag::PromotionCaptureQueen, target));
                }
                else
                {
                    moveList.add(
                        Move(from,
                            to,
                            pawn,
                            MoveFlag::Capture,
                            target));
                }
            }
        }

        //-------------------------------------------------
        // En passant
        //-------------------------------------------------

        Square enPassant = board.getEnPassantSquare();

        if (enPassant != Square::None)
        {
            Bitboard attacks =
                (side == ChessColor::White)
                ? AttackTables::whitePawnAttacks(from)
                : AttackTables::blackPawnAttacks(from);

            if (getBit(attacks, enPassant))
            {
                // Check if en passant is allowed by pin/eviction constraints
                bool epAllowed = true;
                if (checkInfo.pinned & (1ULL << static_cast<int>(from)))
                {
                    // Piece is pinned, check if en passant is along pin ray
                    if (!(checkInfo.pinRays[static_cast<int>(from)] & (1ULL << static_cast<int>(enPassant))))
                    {
                        epAllowed = false;
                    }
                }
                else if (checkInfo.inCheck)
                {
                    // In check, en passant only allowed if it captures a checker
                    int enPassantFile = static_cast<int>(enPassant) % 8;
                    int enPassantRank = static_cast<int>(enPassant) / 8;
                    int capturedFile = enPassantFile;
                    int capturedRank = (side == ChessColor::White) ? enPassantRank - 1 : enPassantRank + 1;
                    Square capturedSquare = static_cast<Square>(capturedRank * 8 + capturedFile);

                    // The captured pawn is not on the en passant square, it's beside it
                    // So we need to check if the captured pawn is a checker
                    if (!(checkInfo.checkers & (1ULL << static_cast<int>(capturedSquare))))
                    {
                        epAllowed = false;
                    }
                }

                if (epAllowed)
                {
                    Piece capturedPawn = (side == ChessColor::White) ? Piece::BlackPawn : Piece::WhitePawn;
                    moveList.add(
                        Move(
                            from,
                            enPassant,
                            pawn,
                            MoveFlag::EnPassant,
                            capturedPawn));
                }
            }
        }
    }
}

void MoveGenerator::generateKnightMoves(
    const Board& board,
    const Bitboards& bitboards,
    MoveList& moveList,
    const MoveValidator::CheckInfo& checkInfo)
{
    ChessColor side = board.getSideToMove();
    Bitboard ownOccupancy = bitboards.occupied[sideIndex(side)];

    Piece knight =
        (side == ChessColor::White)
        ? Piece::WhiteKnight
        : Piece::BlackKnight;

    Bitboard knights = bitboards.knights[sideIndex(side)];

    while (knights)
    {
        Square from = popLeastSignificantBit(knights);

        Bitboard attacks =
            AttackTables::knightAttacks(from);

        // Remove squares occupied by our own pieces
        attacks &= ~ownOccupancy;

        // Apply pin/eviction constraints
        if (checkInfo.pinned & (1ULL << static_cast<int>(from)))
        {
            // Piece is pinned - knights can't be pinned in chess, but handle anyway
            attacks &= checkInfo.pinRays[static_cast<int>(from)];
        }
        else if (checkInfo.inCheck)
        {
            // In check, knight moves only allowed if they capture a checker
            // (knights can't block)
            Bitboard checkerSquares = checkInfo.checkers;
            attacks &= checkerSquares;
        }

        while (attacks)
        {
            Square to = popLeastSignificantBit(attacks);

            Piece target = board.pieceAt(to);

            MoveFlag flag =
                (target == Piece::None)
                ? MoveFlag::Quiet
                : MoveFlag::Capture;

            moveList.add(Move(from, to, knight, flag, target));
        }
    }
}

void MoveGenerator::generateBishopMoves(
    const Board& board,
    const Bitboards& bitboards,
    MoveList& moveList,
    const MoveValidator::CheckInfo& checkInfo)
{
    ChessColor side = board.getSideToMove();
    Bitboard ownOccupancy = bitboards.occupied[sideIndex(side)];
    Bitboard allOccupancy = bitboards.allOccupied;

    Piece bishop =
        (side == ChessColor::White)
        ? Piece::WhiteBishop
        : Piece::BlackBishop;

    Bitboard bishops = bitboards.bishops[sideIndex(side)];

    while (bishops)
    {
        Square from = popLeastSignificantBit(bishops);

        Bitboard attacks = AttackTables::bishopAttacks(from, allOccupancy);
        attacks &= ~ownOccupancy;

        // Apply pin/eviction constraints
        if (checkInfo.pinned & (1ULL << static_cast<int>(from)))
        {
            // Piece is pinned, restrict to pin ray
            attacks &= checkInfo.pinRays[static_cast<int>(from)];
        }
        else if (checkInfo.inCheck)
        {
            // In check, only moves that capture checkers or block are allowed
            Bitboard allowedSquares = checkInfo.checkers; // Can capture checkers

            // Add blocking squares for sliding checkers
            Bitboard checkers = checkInfo.checkers;
            while (checkers)
            {
                Square checkerSq = popLeastSignificantBit(checkers);
                // Only consider sliding pieces (bishop, rook, queen) for blocking
                Piece checkerPiece = board.pieceAt(checkerSq);
                if (checkerPiece == Piece::WhiteBishop || checkerPiece == Piece::BlackBishop ||
                    checkerPiece == Piece::WhiteRook || checkerPiece == Piece::BlackRook ||
                    checkerPiece == Piece::WhiteQueen || checkerPiece == Piece::BlackQueen)
                {
                    // Add squares between king and checker (exclusive)
                    Bitboard between = getBetweenRay(checkInfo.kingSquare, checkerSq);
                    allowedSquares |= between;
                }
            }
            attacks &= allowedSquares;
        }

        while (attacks)
        {
            Square to = popLeastSignificantBit(attacks);

            Piece target = board.pieceAt(to);

            MoveFlag flag =
                (target == Piece::None)
                ? MoveFlag::Quiet
                : MoveFlag::Capture;

            moveList.add(Move(from, to, bishop, flag, target));
        }
    }
}

void MoveGenerator::generateRookMoves(
    const Board& board,
    const Bitboards& bitboards,
    MoveList& moveList,
    const MoveValidator::CheckInfo& checkInfo)
{
    ChessColor side = board.getSideToMove();
    Bitboard ownOccupancy = bitboards.occupied[sideIndex(side)];
    Bitboard allOccupancy = bitboards.allOccupied;

    Piece rook =
        (side == ChessColor::White)
        ? Piece::WhiteRook
        : Piece::BlackRook;

    Bitboard rooks = bitboards.rooks[sideIndex(side)];

    while (rooks)
    {
        Square from = popLeastSignificantBit(rooks);

        Bitboard attacks = AttackTables::rookAttacks(from, allOccupancy);
        attacks &= ~ownOccupancy;

        // Apply pin/eviction constraints
        if (checkInfo.pinned & (1ULL << static_cast<int>(from)))
        {
            // Piece is pinned, restrict to pin ray
            attacks &= checkInfo.pinRays[static_cast<int>(from)];
        }
        else if (checkInfo.inCheck)
        {
            // In check, only moves that capture checkers or block are allowed
            Bitboard allowedSquares = checkInfo.checkers; // Can capture checkers

            // Add blocking squares for sliding checkers
            Bitboard checkers = checkInfo.checkers;
            while (checkers)
            {
                Square checkerSq = popLeastSignificantBit(checkers);
                // Only consider sliding pieces (bishop, rook, queen) for blocking
                Piece checkerPiece = board.pieceAt(checkerSq);
                if (checkerPiece == Piece::WhiteBishop || checkerPiece == Piece::BlackBishop ||
                    checkerPiece == Piece::WhiteRook || checkerPiece == Piece::BlackRook ||
                    checkerPiece == Piece::WhiteQueen || checkerPiece == Piece::BlackQueen)
                {
                    // Add squares between king and checker (exclusive)
                    Bitboard between = getBetweenRay(checkInfo.kingSquare, checkerSq);
                    allowedSquares |= between;
                }
            }
            attacks &= allowedSquares;
        }

        while (attacks)
        {
            Square to = popLeastSignificantBit(attacks);

            Piece target = board.pieceAt(to);

            MoveFlag flag =
                (target == Piece::None)
                ? MoveFlag::Quiet
                : MoveFlag::Capture;

            moveList.add(Move(from, to, rook, flag, target));
        }
    }
}

void MoveGenerator::generateQueenMoves(
    const Board& board,
    const Bitboards& bitboards,
    MoveList& moveList,
    const MoveValidator::CheckInfo& checkInfo)
{
    ChessColor side = board.getSideToMove();
    Bitboard ownOccupancy = bitboards.occupied[sideIndex(side)];
    Bitboard allOccupancy = bitboards.allOccupied;

    Piece queen =
        (side == ChessColor::White)
        ? Piece::WhiteQueen
        : Piece::BlackQueen;

    Bitboard queens = bitboards.queens[sideIndex(side)];

    while (queens)
    {
        Square from = popLeastSignificantBit(queens);

        Bitboard attacks = AttackTables::queenAttacks(from, allOccupancy);
        attacks &= ~ownOccupancy;

        // Apply pin/eviction constraints
        if (checkInfo.pinned & (1ULL << static_cast<int>(from)))
        {
            // Piece is pinned, restrict to pin ray
            attacks &= checkInfo.pinRays[static_cast<int>(from)];
        }
        else if (checkInfo.inCheck)
        {
            // In check, only moves that capture checkers or block are allowed
            Bitboard allowedSquares = checkInfo.checkers; // Can capture checkers

            // Add blocking squares for sliding checkers
            Bitboard checkers = checkInfo.checkers;
            while (checkers)
            {
                Square checkerSq = popLeastSignificantBit(checkers);
                // Only consider sliding pieces (bishop, rook, queen) for blocking
                Piece checkerPiece = board.pieceAt(checkerSq);
                if (checkerPiece == Piece::WhiteBishop || checkerPiece == Piece::BlackBishop ||
                    checkerPiece == Piece::WhiteRook || checkerPiece == Piece::BlackRook ||
                    checkerPiece == Piece::WhiteQueen || checkerPiece == Piece::BlackQueen)
                {
                    // Add squares between king and checker (exclusive)
                    Bitboard between = getBetweenRay(checkInfo.kingSquare, checkerSq);
                    allowedSquares |= between;
                }
            }
            attacks &= allowedSquares;
        }

        while (attacks)
        {
            Square to = popLeastSignificantBit(attacks);

            Piece target = board.pieceAt(to);

            MoveFlag flag =
                (target == Piece::None)
                ? MoveFlag::Quiet
                : MoveFlag::Capture;

            moveList.add(Move(from, to, queen, flag, target));
        }
    }
}

void MoveGenerator::generateKingMoves(
    const Board& board,
    const Bitboards& bitboards,
    MoveList& moveList,
    const MoveValidator::CheckInfo& checkInfo)
{
    ChessColor side = board.getSideToMove();
    Bitboard ownOccupancy = bitboards.occupied[sideIndex(side)];

    Piece king =
        (side == ChessColor::White)
        ? Piece::WhiteKing
        : Piece::BlackKing;

    Bitboard kings = bitboards.kings[sideIndex(side)];

    while (kings)
    {
        Square from = popLeastSignificantBit(kings);

        Bitboard attacks =
            AttackTables::kingAttacks(from);

        // Remove squares occupied by our own pieces
        attacks &= ~ownOccupancy;

        // Remove squares attacked by enemy (unless capturing that piece)
        Bitboard safeSquares = attacks & ~checkInfo.enemyAttacks;

        // Add back squares where we capture enemy pieces (even if they attack that square)
Bitboard enemyOccupancy = bitboards.occupied[sideIndex(MoveValidator::oppositeColor(side))];
        Bitboard capturableEnemy = safeSquares | (attacks & enemyOccupancy);

        // Additional restriction: if in check, king can only move to squares that evade check
        if (checkInfo.inCheck)
        {
            // King moves are only allowed to squares not attacked by enemy
            // (except when capturing the checking piece)
            Bitboard evasionSquares = ~checkInfo.enemyAttacks;

            // Add squares where we capture checking pieces
            Bitboard checkers = checkInfo.checkers;
            while (checkers)
            {
                Square checkerSq = popLeastSignificantBit(checkers);
                evasionSquares |= (1ULL << static_cast<int>(checkerSq));
            }

            capturableEnemy &= evasionSquares;
        }

        while (capturableEnemy)
        {
            Square to = popLeastSignificantBit(capturableEnemy);

            Piece target = board.pieceAt(to);

            // Validate king capture: ensure destination square is not attacked by enemy
            // pieces other than the one being captured
            if (target != Piece::None)
            {
                // Create modified occupancy without the target piece
                Bitboard modifiedOccupancy = board.getAllOccupancy();
                clearBit(modifiedOccupancy, to);

                // Check if destination square is attacked by enemy sliding pieces with modified occupancy
                ChessColor enemy = MoveValidator::oppositeColor(side);

                Bitboard enemyRooks = board.getBitboard(enemy == ChessColor::White ? Piece::WhiteRook : Piece::BlackRook);
                Bitboard enemyQueens = board.getBitboard(enemy == ChessColor::White ? Piece::WhiteQueen : Piece::BlackQueen);
                Bitboard enemyBishops = board.getBitboard(enemy == ChessColor::White ? Piece::WhiteBishop : Piece::BlackBishop);

                // Remove target piece from enemy sliders if it's a slider
                Bitboard enemyOrthogonal = enemyRooks | enemyQueens;
                Bitboard enemyDiagonal = enemyBishops | enemyQueens;

                if (target == (enemy == ChessColor::White ? Piece::WhiteRook : Piece::BlackRook) ||
                    target == (enemy == ChessColor::White ? Piece::WhiteQueen : Piece::BlackQueen))
                {
                    clearBit(enemyOrthogonal, to);
                }
                if (target == (enemy == ChessColor::White ? Piece::WhiteBishop : Piece::BlackBishop) ||
                    target == (enemy == ChessColor::White ? Piece::WhiteQueen : Piece::BlackQueen))
                {
                    clearBit(enemyDiagonal, to);
                }

                Bitboard enemySliderAttacks = 0;
                enemySliderAttacks |= AttackTables::rookAttacks(to, modifiedOccupancy & (enemyRooks | enemyQueens));
                enemySliderAttacks |= AttackTables::bishopAttacks(to, modifiedOccupancy & (enemyBishops | enemyQueens));

                // Pawn, knight, king attacks (not affected by occupancy)
                Bitboard enemyPawnAttacks = (enemy == ChessColor::White)
                    ? AttackTables::whitePawnAttacks(to)
                    : AttackTables::blackPawnAttacks(to);
                Bitboard enemyKnightAttacks = AttackTables::knightAttacks(to);
                Bitboard enemyKingAttacks = AttackTables::kingAttacks(to);

                Bitboard enemyAttacks = enemySliderAttacks | enemyPawnAttacks | enemyKnightAttacks | enemyKingAttacks;

                // If destination is still attacked, this capture is illegal
                if (enemyAttacks & (1ULL << static_cast<int>(to)))
                {
                    continue; // Skip this move
                }
            }

            MoveFlag flag =
                (target == Piece::None)
                ? MoveFlag::Quiet
                : MoveFlag::Capture;

            moveList.add(
                Move(from, to, king, flag, target));
        }

        //--------------------------------------------------
        // Castling - w jednym if-else if dla obu kolorów
        //--------------------------------------------------

        if (side == ChessColor::White)
        {
            // King side castle
            if ((board.getCastlingRights() & 0b0001) &&
                board.pieceAt(Square::H1) == Piece::WhiteRook &&
                board.pieceAt(Square::F1) == Piece::None &&
                board.pieceAt(Square::G1) == Piece::None &&
                // King must not be in check and squares passed through must not be attacked
                !checkInfo.inCheck &&
!getBit(checkInfo.enemyAttacks, Square::F1) &&
            !getBit(checkInfo.enemyAttacks, Square::G1))
            {
                moveList.add(
                    Move(Square::E1, Square::G1,
                         Piece::WhiteKing, MoveFlag::KingCastle));
            }

            // Queen side castle
            if ((board.getCastlingRights() & 0b0010) &&
                board.pieceAt(Square::A1) == Piece::WhiteRook &&
                board.pieceAt(Square::B1) == Piece::None &&
                board.pieceAt(Square::C1) == Piece::None &&
                board.pieceAt(Square::D1) == Piece::None &&
                // King must not be in check and squares passed through must not be attacked
                !checkInfo.inCheck &&
!getBit(checkInfo.enemyAttacks, Square::D1) &&
            !getBit(checkInfo.enemyAttacks, Square::C1))
            {
                moveList.add(
                    Move(Square::E1, Square::C1,
                         Piece::WhiteKing, MoveFlag::QueenCastle));
            }
        }
        else
        {
            // King side castle
            if ((board.getCastlingRights() & 0b0100) &&
                board.pieceAt(Square::H8) == Piece::BlackRook &&
                board.pieceAt(Square::F8) == Piece::None &&
                board.pieceAt(Square::G8) == Piece::None &&
                // King must not be in check and squares passed through must not be attacked
                !checkInfo.inCheck &&
!getBit(checkInfo.enemyAttacks, Square::F8) &&
            !getBit(checkInfo.enemyAttacks, Square::G8))
            {
                moveList.add(
                    Move(Square::E8, Square::G8,
                         Piece::BlackKing, MoveFlag::KingCastle));
            }

            // Queen side castle
            if ((board.getCastlingRights() & 0b1000) &&
                board.pieceAt(Square::A8) == Piece::BlackRook &&
                board.pieceAt(Square::B8) == Piece::None &&
                board.pieceAt(Square::C8) == Piece::None &&
                board.pieceAt(Square::D8) == Piece::None &&
                // King must not be in check and squares passed through must not be attacked
                !checkInfo.inCheck &&
!getBit(checkInfo.enemyAttacks, Square::D8) &&
            !getBit(checkInfo.enemyAttacks, Square::C8))
            {
                moveList.add(
                    Move(Square::E8, Square::C8,
                         Piece::BlackKing, MoveFlag::QueenCastle));
            }
        }
    }
}

// Helper functions for capture generation

void MoveGenerator::generatePawnCaptures(
    const Board& board,
    const Bitboards& bitboards,
    MoveList& moveList,
    const MoveValidator::CheckInfo& checkInfo)
{
    // For captures, we reuse the pawn move generation but filter for captures only
    MoveList allPawnMoves;
    generatePawnMoves(board, bitboards, allPawnMoves, checkInfo);

    for (int i = 0; i < allPawnMoves.size(); i++)
    {
        const Move& move = allPawnMoves[i];
        if (move.flag == MoveFlag::Capture ||
            move.flag >= MoveFlag::PromotionCaptureKnight ||
            move.flag == MoveFlag::EnPassant)
        {
            moveList.add(move);
        }
    }
}

void MoveGenerator::generateKnightCaptures(
    const Board& board,
    const Bitboards& bitboards,
    MoveList& moveList,
    const MoveValidator::CheckInfo& checkInfo)
{
    MoveList allKnightMoves;
    generateKnightMoves(board, bitboards, allKnightMoves, checkInfo);

    for (int i = 0; i < allKnightMoves.size(); i++)
    {
        const Move& move = allKnightMoves[i];
        if (move.flag == MoveFlag::Capture)
        {
            moveList.add(move);
        }
    }
}

void MoveGenerator::generateBishopCaptures(
    const Board& board,
    const Bitboards& bitboards,
    MoveList& moveList,
    const MoveValidator::CheckInfo& checkInfo)
{
    MoveList allBishopMoves;
    generateBishopMoves(board, bitboards, allBishopMoves, checkInfo);

    for (int i = 0; i < allBishopMoves.size(); i++)
    {
        const Move& move = allBishopMoves[i];
        if (move.flag == MoveFlag::Capture)
        {
            moveList.add(move);
        }
    }
}

void MoveGenerator::generateRookCaptures(
    const Board& board,
    const Bitboards& bitboards,
    MoveList& moveList,
    const MoveValidator::CheckInfo& checkInfo)
{
    MoveList allRookMoves;
    generateRookMoves(board, bitboards, allRookMoves, checkInfo);

    for (int i = 0; i < allRookMoves.size(); i++)
    {
        const Move& move = allRookMoves[i];
        if (move.flag == MoveFlag::Capture)
        {
            moveList.add(move);
        }
    }
}

void MoveGenerator::generateQueenCaptures(
    const Board& board,
    const Bitboards& bitboards,
    MoveList& moveList,
    const MoveValidator::CheckInfo& checkInfo)
{
    MoveList allQueenMoves;
    generateQueenMoves(board, bitboards, allQueenMoves, checkInfo);

    for (int i = 0; i < allQueenMoves.size(); i++)
    {
        const Move& move = allQueenMoves[i];
        if (move.flag == MoveFlag::Capture)
        {
            moveList.add(move);
        }
    }
}

void MoveGenerator::generateKingCaptures(
    const Board& board,
    const Bitboards& bitboards,
    MoveList& moveList,
    const MoveValidator::CheckInfo& checkInfo)
{
    MoveList allKingMoves;
    generateKingMoves(board, bitboards, allKingMoves, checkInfo);

    for (int i = 0; i < allKingMoves.size(); i++)
    {
        const Move& move = allKingMoves[i];
        if (move.flag == MoveFlag::Capture)
        {
            moveList.add(move);
        }
    }
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
