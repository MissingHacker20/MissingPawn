#include "MoveGenerator.h"

#include "AttackTables.h"
#include "Bitboard.h"
#include "Move.h"
#include "MoveValidator.h"

void MoveGenerator::generateMoves(
    const Board& board,
    MoveList& moveList)
{
    moveList.clear();

    MoveList pseudoMoves;

    generatePawnMoves(board, pseudoMoves);

    generateKnightMoves(board, pseudoMoves);

    generateBishopMoves(board, pseudoMoves);

    generateRookMoves(board, pseudoMoves);

    generateQueenMoves(board, pseudoMoves);

    generateKingMoves(board, pseudoMoves);

    //--------------------------------------------------
    // Keep only legal moves
    //--------------------------------------------------

    for (int i = 0; i < pseudoMoves.size(); i++)
    {
        const Move& move = pseudoMoves[i];

        if (MoveValidator::isMoveLegal(board, move))
        {
            moveList.add(move);
        }
    }
}

void MoveGenerator::generateCaptures(
    const Board& board,
    MoveList& captures)
{
    captures.clear();

    MoveList moves;

    generateMoves(board, moves);

    for (int i = 0; i < moves.size(); i++)
    {
        if (moves[i].capturedPiece != Piece::None)
        {
            captures.add(moves[i]);
        }
    }
}

void MoveGenerator::generatePawnMoves(
    const Board& board,
    MoveList& moveList)
{
    ChessColor side = board.getSideToMove();

    Piece pawn =
        (side == ChessColor::White)
        ? Piece::WhitePawn
        : Piece::BlackPawn;

    Bitboard pawns = board.getBitboard(pawn);

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

            if (board.pieceAt(to) == Piece::None)
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
                        Move(from, to, pawn, MoveFlag::Quiet)
                    );

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

                        if (board.pieceAt(second) == Piece::None)
                        {
                            moveList.add(
                                Move(from, second, pawn, MoveFlag::DoublePawnPush));
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
                Piece capturedPawn =
                    (side == ChessColor::White)
                    ? Piece::BlackPawn
                    : Piece::WhitePawn;

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

void MoveGenerator::generateKnightMoves(
    const Board& board,
    MoveList& moveList)
{
    ChessColor side = board.getSideToMove();

    Piece knight =
        (side == ChessColor::White)
        ? Piece::WhiteKnight
        : Piece::BlackKnight;

    Bitboard knights = board.getBitboard(knight);

    while (knights)
    {
        Square from = popLeastSignificantBit(knights);

        Bitboard attacks =
            AttackTables::knightAttacks(from);

        // Remove squares occupied by our own pieces
        attacks &= ~board.getOccupancy(side);

        while (attacks)
        {
            Square to = popLeastSignificantBit(attacks);
            
            Piece target = board.pieceAt(to);

            MoveFlag flag =
                (board.pieceAt(to) == Piece::None)
                ? MoveFlag::Quiet
                : MoveFlag::Capture;

            moveList.add(Move(from, to, knight, flag, target));
        }
    }
}

void MoveGenerator::generateBishopMoves(
    const Board& board,
    MoveList& moveList)
{
    ChessColor side = board.getSideToMove();

    Piece bishop =
        (side == ChessColor::White)
        ? Piece::WhiteBishop
        : Piece::BlackBishop;

    Bitboard bishops = board.getBitboard(bishop);

    constexpr int Directions[4][2] =
    {
        { 1,  1},
        { 1, -1},
        {-1, -1},
        {-1,  1}
    };

    while (bishops)
    {
        Square from = popLeastSignificantBit(bishops);

        int file = static_cast<int>(from) % 8;
        int rank = static_cast<int>(from) / 8;

        for (int dir = 0; dir < 4; dir++)
        {
            int newFile = file;
            int newRank = rank;

            while (true)
            {
                newFile += Directions[dir][0];
                newRank += Directions[dir][1];

                if (newFile < 0 || newFile >= 8 ||
                    newRank < 0 || newRank >= 8)
                {
                    break;
                }

                Square to =
                    static_cast<Square>(newRank * 8 + newFile);

                Piece target = board.pieceAt(to);

                if (target == Piece::None)
                {
                    moveList.add(
                        Move(from, to, bishop, MoveFlag::Quiet)
                    );

                    continue;
                }

                if (getPieceColor(target) != side)
                {
                    moveList.add(Move(from, to, bishop, MoveFlag::Capture, target));
                }

                break;
            }
        }
    }
}

void MoveGenerator::generateRookMoves(
    const Board& board,
    MoveList& moveList)
{
    ChessColor side = board.getSideToMove();

    Piece rook =
        (side == ChessColor::White)
        ? Piece::WhiteRook
        : Piece::BlackRook;

    Bitboard rooks = board.getBitboard(rook);

    constexpr int Directions[4][2] =
    {
        { 0,  1},
        { 1,  0},
        { 0, -1},
        {-1,  0}
    };

    while (rooks)
    {
        Square from = popLeastSignificantBit(rooks);

        int file = static_cast<int>(from) % 8;
        int rank = static_cast<int>(from) / 8;

        for (int dir = 0; dir < 4; dir++)
        {
            int newFile = file;
            int newRank = rank;

            while (true)
            {
                newFile += Directions[dir][0];
                newRank += Directions[dir][1];

                if (newFile < 0 || newFile >= 8 ||
                    newRank < 0 || newRank >= 8)
                {
                    break;
                }

                Square to =
                    static_cast<Square>(newRank * 8 + newFile);

                Piece target = board.pieceAt(to);

                if (target == Piece::None)
                {
                    moveList.add(
                        Move(from, to, rook, MoveFlag::Quiet)
                    );

                    continue;
                }

                if (getPieceColor(target) != side)
                {
                    moveList.add(
                        Move(from, to, rook, MoveFlag::Capture, target)
                    );
                }

                break;
            }
        }
    }
}

void MoveGenerator::generateQueenMoves(
    const Board& board,
    MoveList& moveList)
{
    ChessColor side = board.getSideToMove();

    Piece queen =
        (side == ChessColor::White)
        ? Piece::WhiteQueen
        : Piece::BlackQueen;

    Bitboard queens = board.getBitboard(queen);

    constexpr int Directions[8][2] =
    {
        // Rook directions
        { 0,  1},
        { 1,  0},
        { 0, -1},
        {-1,  0},

        // Bishop directions
        { 1,  1},
        { 1, -1},
        {-1, -1},
        {-1,  1}
    };

    while (queens)
    {
        Square from = popLeastSignificantBit(queens);

        int file = static_cast<int>(from) % 8;
        int rank = static_cast<int>(from) / 8;

        for (int dir = 0; dir < 8; dir++)
        {
            int newFile = file;
            int newRank = rank;

            while (true)
            {
                newFile += Directions[dir][0];
                newRank += Directions[dir][1];

                if (newFile < 0 || newFile >= 8 ||
                    newRank < 0 || newRank >= 8)
                {
                    break;
                }

                Square to =
                    static_cast<Square>(newRank * 8 + newFile);

                Piece target = board.pieceAt(to);

                if (target == Piece::None)
                {
                    moveList.add(
                        Move(from,
                             to,
                             queen,
                             MoveFlag::Quiet)
                    );

                    continue;
                }

                if (getPieceColor(target) != side)
                {
                    moveList.add(
                        Move(from,
                             to,
                             queen,
                             MoveFlag::Capture,
                             target)
                    );
                }

                break;
            }
        }
    }
}

void MoveGenerator::generateKingMoves(
    const Board& board,
    MoveList& moveList)
{
    ChessColor side = board.getSideToMove();

    Piece king =
        (side == ChessColor::White)
        ? Piece::WhiteKing
        : Piece::BlackKing;

    Bitboard kings = board.getBitboard(king);

    while (kings)
    {
        Square from = popLeastSignificantBit(kings);

        Bitboard attacks =
            AttackTables::kingAttacks(from);

        // Usuń własne figury
        attacks &= ~board.getOccupancy(side);

        while (attacks)
        {
            Square to = popLeastSignificantBit(attacks);

            Piece target = board.pieceAt(to);

            MoveFlag flag =
                (target == Piece::None)
                ? MoveFlag::Quiet
                : MoveFlag::Capture;

            moveList.add(
                Move(from, to, king, flag, target));
        }
        //--------------------------------------------------
        // King side castle
        //--------------------------------------------------

        if (side == ChessColor::White)
        {
            if (board.getCastlingRights() & 0b0001)
            {
               if (board.pieceAt(Square::H1) == Piece::WhiteRook &&
                    board.pieceAt(Square::F1) == Piece::None &&
                    board.pieceAt(Square::G1) == Piece::None)
                {
                    moveList.add(
                        Move(
                        Square::E1,
                        Square::G1,
                        Piece::WhiteKing,
                        MoveFlag::KingCastle));
                }
            }
        }
        else
        {
            if (board.getCastlingRights() & 0b0100)
            {
                if (board.pieceAt(Square::H8) == Piece::BlackRook &&
                    board.pieceAt(Square::F8) == Piece::None &&
                    board.pieceAt(Square::G8) == Piece::None)
                {
                    moveList.add(
                        Move(
                            Square::E8,
                            Square::G8,
                            Piece::BlackKing,
                            MoveFlag::KingCastle));
                }
            }
        }
        //--------------------------------------------------
        // Queen side castle
        //--------------------------------------------------

        if (side == ChessColor::White)
        {
            if (board.getCastlingRights() & 0b0010)
            {
                if (board.pieceAt(Square::A1) == Piece::WhiteRook &&
                    board.pieceAt(Square::B1) == Piece::None &&
                    board.pieceAt(Square::C1) == Piece::None &&
                    board.pieceAt(Square::D1) == Piece::None)
                {
                    moveList.add(
                        Move(
                            Square::E1,
                            Square::C1,
                            Piece::WhiteKing,
                            MoveFlag::QueenCastle));
                }
            }
        }
        else
        {
            if (board.getCastlingRights() & 0b1000)
            {
                if (board.pieceAt(Square::A8) == Piece::BlackRook &&
                    board.pieceAt(Square::B8) == Piece::None &&
                    board.pieceAt(Square::C8) == Piece::None &&
                    board.pieceAt(Square::D8) == Piece::None)
                {
                    moveList.add(
                        Move(
                            Square::E8,
                            Square::C8,
                            Piece::BlackKing,
                            MoveFlag::QueenCastle));
                }
            }
        }

    }
}
