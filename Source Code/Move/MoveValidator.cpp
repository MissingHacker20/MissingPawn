#include "MoveValidator.h"
#include "Board.h"
#include "AttackTables.h"

void MoveValidator::filterLegalMoves(
    const Board& board,
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
    const Board& board,
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

        Board copy = board;
        UndoInfo undo;

        Move stepMove(
            move.from,
            (side == ChessColor::White) ? Square::F1 : Square::F8,
            move.piece,
            MoveFlag::Quiet);

        copy.makeMove(stepMove, undo);

        if (isKingInCheck(copy, side))
        {
            return false;
        }
    }

    if (move.flag == MoveFlag::QueenCastle)
    {
        ChessColor side = board.getSideToMove();

        if (isKingInCheck(board, side))
        {
            return false;
        }

        Board copy = board;
        UndoInfo undo;

        Move stepMove(
            move.from,
            (side == ChessColor::White) ? Square::D1 : Square::D8,
            move.piece,
            MoveFlag::Quiet);

        copy.makeMove(stepMove, undo);

        if (isKingInCheck(copy, side))
        {
            return false;
        }
    }

    Board copy = board;

    UndoInfo undoInfo;

    copy.makeMove(move, undoInfo);

    return !isKingInCheck(
        copy,
        board.getSideToMove());
}

bool MoveValidator::isSquareAttacked(
    const Board& board,
    Square square,
    ChessColor attacker)
{
    //--------------------------------------------------
    // Pawn attacks
    //--------------------------------------------------

    Bitboard attacks =
        (attacker == ChessColor::White)
        ? AttackTables::blackPawnAttacks(square)
        : AttackTables::whitePawnAttacks(square);

    Piece pawn =
        (attacker == ChessColor::White)
        ? Piece::WhitePawn
        : Piece::BlackPawn;

    if (attacks & board.getBitboard(pawn))
    {
        return true;
    }
    //--------------------------------------------------
    // Knight attacks
    //--------------------------------------------------

    Bitboard knightAttacks =
    AttackTables::knightAttacks(square);

    Piece knight =
        (attacker == ChessColor::White)
        ? Piece::WhiteKnight
        : Piece::BlackKnight;

    if (knightAttacks & board.getBitboard(knight))
    {
    return true;
    }
    //--------------------------------------------------
    // King attacks
    //--------------------------------------------------

    Bitboard kingAttacks =
    AttackTables::kingAttacks(square);

    Piece king =
        (attacker == ChessColor::White)
        ? Piece::WhiteKing
        : Piece::BlackKing;

    if (kingAttacks & board.getBitboard(king))
    {
        return true;
    }
    //--------------------------------------------------
    // Rook / Queen attacks (slide until we hit a piece)
    //--------------------------------------------------

    constexpr int RookDirections[4][2] =
    {
        { 0,  1},
        { 1,  0},
        { 0, -1},
        {-1,  0}
    };

    for (int dir = 0; dir < 4; dir++)
    {
        int file = static_cast<int>(square) % 8;
        int rank = static_cast<int>(square) / 8;

        while (true)
        {
            file += RookDirections[dir][0];
            rank += RookDirections[dir][1];

            if (file < 0 || file >= 8 ||
            rank < 0 || rank >= 8)
            {
                break;
            }

            Square target =
                static_cast<Square>(rank * 8 + file);

            Piece piece = board.pieceAt(target);

            if (piece == Piece::None)
            {
                continue;
            }

            if (getPieceColor(piece) == attacker)
            {
                if (piece ==
                    (attacker == ChessColor::White
                        ? Piece::WhiteRook
                        : Piece::BlackRook))
                {
                    return true;
                }

                if (piece ==
                    (attacker == ChessColor::White
                        ? Piece::WhiteQueen
                        : Piece::BlackQueen))
                {
                    return true;
                }
            }

            // Stop at any piece (friendly or enemy) - pieces block line of sight
            break;
        }
    }
    //--------------------------------------------------
    // Bishop / Queen attacks (slide until we hit a piece)
    //--------------------------------------------------

    constexpr int BishopDirections[4][2] =
    {
        { 1,  1},
        { 1, -1},
        {-1, -1},
        {-1,  1}
    };

    for (int dir = 0; dir < 4; dir++)
    {
        int file = static_cast<int>(square) % 8;
        int rank = static_cast<int>(square) / 8;

        while (true)
        {
            file += BishopDirections[dir][0];
            rank += BishopDirections[dir][1];

            if (file < 0 || file >= 8 ||
                rank < 0 || rank >= 8)
            {
                break;
            }

            Square target =
            static_cast<Square>(rank * 8 + file);

            Piece piece = board.pieceAt(target);

            if (piece == Piece::None)
            {
                continue;
            }

            if (getPieceColor(piece) == attacker)
            {
                if (piece ==
                    (attacker == ChessColor::White
                        ? Piece::WhiteBishop
                        : Piece::BlackBishop))
                {
                    return true;
                }

                if (piece ==
                    (attacker == ChessColor::White
                        ? Piece::WhiteQueen
                        : Piece::BlackQueen))
                {
                    return true;
                }
            }

            // Stop at any piece (friendly or enemy) - pieces block line of sight
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
