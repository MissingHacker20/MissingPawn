#include "Move/MoveParser.h"

#include <cctype>

#include "Foundation/Board.h"
#include "Foundation/Square.h"
#include "Move/MoveGenerator.h"
#include "Move/MoveList.h"

bool MoveParser::parseMove(
    const Board& board,
    const std::string& uciString,
    Move& outMove)
{
    if (uciString.length() < 4 || uciString.length() > 5)
    {
        return false;
    }

    // Parsuj współrzędne
    std::string fromStr = uciString.substr(0, 2);
    std::string toStr = uciString.substr(2, 2);

    Square from = stringToSquare(fromStr);
    Square to = stringToSquare(toStr);

    if (from == Square::None || to == Square::None)
    {
        return false;
    }

    Piece piece = board.pieceAt(from);

    if (piece == Piece::None)
    {
        return false;
    }

    // Ustal flagę podstawową
    MoveFlag flag = MoveFlag::Quiet;
    Piece captured = board.pieceAt(to);

    if (captured != Piece::None)
    {
        flag = MoveFlag::Capture;
    }

    // Sprawdź promocję
    if (uciString.length() == 5)
    {
        char promoChar = std::tolower(uciString[4]);
        
        if (piece != Piece::WhitePawn && piece != Piece::BlackPawn)
        {
            return false;
        }

        // Ustaw odpowiednią flagę promocji
        if (captured != Piece::None)
        {
            switch (promoChar)
            {
            case 'n': flag = MoveFlag::PromotionCaptureKnight; break;
            case 'b': flag = MoveFlag::PromotionCaptureBishop; break;
            case 'r': flag = MoveFlag::PromotionCaptureRook; break;
            case 'q': flag = MoveFlag::PromotionCaptureQueen; break;
            default: return false;
            }
        }
        else
        {
            switch (promoChar)
            {
            case 'n': flag = MoveFlag::PromotionKnight; break;
            case 'b': flag = MoveFlag::PromotionBishop; break;
            case 'r': flag = MoveFlag::PromotionRook; break;
            case 'q': flag = MoveFlag::PromotionQueen; break;
            default: return false;
            }
        }
    }

    // Sprawdź en passant
    if ((piece == Piece::WhitePawn || piece == Piece::BlackPawn) &&
        board.getEnPassantSquare() == to)
    {
        flag = MoveFlag::EnPassant;
        captured = (piece == Piece::WhitePawn) ? Piece::BlackPawn : Piece::WhitePawn;
    }

    // Sprawdź roszadę
    if (piece == Piece::WhiteKing && from == Square::E1)
    {
        if (to == Square::G1) flag = MoveFlag::KingCastle;
        else if (to == Square::C1) flag = MoveFlag::QueenCastle;
    }
    if (piece == Piece::BlackKing && from == Square::E8)
    {
        if (to == Square::G8) flag = MoveFlag::KingCastle;
        else if (to == Square::C8) flag = MoveFlag::QueenCastle;
    }

    // Sprawdź double pawn push
    if (piece == Piece::WhitePawn && static_cast<int>(to) == static_cast<int>(from) + 16)
    {
        flag = MoveFlag::DoublePawnPush;
    }
    if (piece == Piece::BlackPawn && static_cast<int>(to) == static_cast<int>(from) - 16)
    {
        flag = MoveFlag::DoublePawnPush;
    }

    outMove = Move(from, to, piece, flag, captured);
    return true;
}

std::vector<Move> MoveParser::parseMoveList(
    const Board& board,
    const std::vector<std::string>& uciMoves)
{
    std::vector<Move> moves;
    Board tempBoard = board;

    for (const std::string& uci : uciMoves)
    {
        Move move;
        if (parseMove(tempBoard, uci, move))
        {
            moves.push_back(move);

            UndoInfo undoInfo;
            tempBoard.makeMove(move, undoInfo);
        }
        else
        {
            break;
        }
    }

    return moves;
}
