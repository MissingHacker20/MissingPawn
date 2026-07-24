#include "Move/MoveParser.h"

#include "Foundation/Board.h"
#include "Foundation/Move.h"
#include "Foundation/Square.h"
#include "Move/MoveGenerator.h"
#include "Move/MoveList.h"
#include "Move/MoveValidator.h"

bool MoveParser::parseMove(
    const Board& board,
    const std::string& uciString,
    Move& outMove)
{
    if (uciString.size() < 4)
    {
        return false;
    }

    //--------------------------------------------------
    // Parse from and to squares
    //--------------------------------------------------

    std::string fromText = uciString.substr(0, 2);
    std::string toText = uciString.substr(2, 2);

    Square from = stringToSquare(fromText);
    Square to = stringToSquare(toText);

    if (from == Square::None || to == Square::None)
    {
        return false;
    }

    //--------------------------------------------------
    // Determine piece
    //--------------------------------------------------

    Piece piece = board.pieceAt(from);

    if (piece == Piece::None)
    {
        return false;
    }

    //--------------------------------------------------
    // Determine flag and captured piece
    //--------------------------------------------------

    MoveFlag flag = MoveFlag::Quiet;
    Piece capturedPiece = board.pieceAt(to);

    // Check if it's a capture
    if (capturedPiece != Piece::None)
    {
        flag = MoveFlag::Capture;
    }

    // Check for en passant
    if (piece == Piece::WhitePawn || piece == Piece::BlackPawn)
    {
        Square enPassant = board.getEnPassantSquare();

        if (to == enPassant && to != Square::None)
        {
            flag = MoveFlag::EnPassant;

            capturedPiece = (piece == Piece::WhitePawn)
                ? Piece::BlackPawn
                : Piece::WhitePawn;
        }
    }

    // Check for double pawn push
    if (piece == Piece::WhitePawn || piece == Piece::BlackPawn)
    {
        int fromRank = static_cast<int>(from) / 8;
        int toRank = static_cast<int>(to) / 8;

        if (std::abs(toRank - fromRank) == 2)
        {
            flag = MoveFlag::DoublePawnPush;
        }
    }

    // Check for promotion
    Piece promotionPiece = Piece::None;

    if (uciString.size() >= 5)
    {
        char promoChar = uciString[4];

        bool isCapture = (flag == MoveFlag::Capture);

        switch (promoChar)
        {
        case 'n':
            flag = isCapture ? MoveFlag::PromotionCaptureKnight : MoveFlag::PromotionKnight;
            promotionPiece = (piece == Piece::WhitePawn) ? Piece::WhiteKnight : Piece::BlackKnight;
            break;

        case 'b':
            flag = isCapture ? MoveFlag::PromotionCaptureBishop : MoveFlag::PromotionBishop;
            promotionPiece = (piece == Piece::WhitePawn) ? Piece::WhiteBishop : Piece::BlackBishop;
            break;

        case 'r':
            flag = isCapture ? MoveFlag::PromotionCaptureRook : MoveFlag::PromotionRook;
            promotionPiece = (piece == Piece::WhitePawn) ? Piece::WhiteRook : Piece::BlackRook;
            break;

        case 'q':
            flag = isCapture ? MoveFlag::PromotionCaptureQueen : MoveFlag::PromotionQueen;
            promotionPiece = (piece == Piece::WhitePawn) ? Piece::WhiteQueen : Piece::BlackQueen;
            break;

        default:
            return false;
        }
    }

    // Check for castling
    if (piece == Piece::WhiteKing || piece == Piece::BlackKing)
    {
        int fromFile = static_cast<int>(from) % 8;
        int toFile = static_cast<int>(to) % 8;

        if (std::abs(toFile - fromFile) == 2)
        {
            if (toFile > fromFile)
            {
                flag = MoveFlag::KingCastle;
            }
            else
            {
                flag = MoveFlag::QueenCastle;
            }

            capturedPiece = Piece::None;
        }
    }

    //--------------------------------------------------
    // Build the move
    //--------------------------------------------------

    outMove = Move(from, to, piece, flag, capturedPiece);

    //--------------------------------------------------
    // Verify the move is legal by generating all moves
    //--------------------------------------------------

    MoveList legalMoves;
    MoveGenerator::generateMoves(board, legalMoves);

    for (int i = 0; i < legalMoves.size(); i++)
    {
        if (legalMoves[i].from == outMove.from &&
            legalMoves[i].to == outMove.to &&
            legalMoves[i].flag == outMove.flag)
        {
            outMove = legalMoves[i];
            return true;
        }
    }

    return false;
}

