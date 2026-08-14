#include "Board.h"
#include "Game/Zobrist.h"


Board::Board()
{
    clear();
}

void Board::clear()
{
    bitboards.fill(0);
    mailbox.fill(Piece::None);

    whiteOccupancy = 0;
    blackOccupancy = 0;
    allOccupancy = 0;

    sideToMove = ChessColor::White;
    castlingRights = 0;
    enPassantSquare = Square::None;

    halfmoveClock = 0;
    fullmoveNumber = 1;

    attackBitboards.fill(0);
    moveBitboards.fill(0);

    updateZobristKey();
}

void Board::setPiece(Piece piece, Square square)
{
    setBit(bitboards[static_cast<int>(piece)], square);
    mailbox[static_cast<int>(square)] = piece;

    updateOccupancy();
}

void Board::removePiece(Piece piece, Square square)
{
    clearBit(bitboards[static_cast<int>(piece)], square);
    mailbox[static_cast<int>(square)] = Piece::None;

    updateOccupancy();
}

void Board::setPieceInternal(Piece piece, Square square)
{
    setBit(bitboards[static_cast<int>(piece)], square);
    mailbox[static_cast<int>(square)] = piece;
}

void Board::removePieceInternal(Piece piece, Square square)
{
    clearBit(bitboards[static_cast<int>(piece)], square);
    mailbox[static_cast<int>(square)] = Piece::None;
}

bool Board::hasPiece(Piece piece, Square square) const
{
    return getBit(bitboards[static_cast<int>(piece)], square);
}

const std::array<Square, 2> WhiteRooks =
{
    Square::A1,
    Square::H1
};

const std::array<Square, 2> WhiteKnights =
{
    Square::B1,
    Square::G1
};

const std::array<Square, 2> WhiteBishops =
{
    Square::C1,
    Square::F1
};

const std::array<Square, 2> BlackRooks =
{
    Square::A8,
    Square::H8
};

const std::array<Square, 2> BlackKnights =
{
    Square::B8,
    Square::G8
};

const std::array<Square, 2> BlackBishops =
{
    Square::C8,
    Square::F8
};

void Board::setStartPosition()
{
    clear();

    // White pawns
    for (int file = 0; file < 8; file++)
    {
        setPiece(Piece::WhitePawn, static_cast<Square>(8 + file));
    }

    // Black pawns
    for (int file = 0; file < 8; file++)
    {
        setPiece(Piece::BlackPawn, static_cast<Square>(48 + file));
    }

    for (Square square : WhiteRooks)
        setPiece(Piece::WhiteRook, square);

    for (Square square : WhiteKnights)
        setPiece(Piece::WhiteKnight, square);

    for (Square square : WhiteBishops)
        setPiece(Piece::WhiteBishop, square);

    setPiece(Piece::WhiteQueen, Square::D1);
    setPiece(Piece::WhiteKing, Square::E1);

    for (Square square : BlackRooks)
        setPiece(Piece::BlackRook, square);

    for (Square square : BlackKnights)
        setPiece(Piece::BlackKnight, square);

    for (Square square : BlackBishops)
        setPiece(Piece::BlackBishop, square);

    setPiece(Piece::BlackQueen, Square::D8);
    setPiece(Piece::BlackKing, Square::E8);

    updateOccupancy();

    sideToMove = ChessColor::White;

    castlingRights = 0b1111;

    enPassantSquare = Square::None;

    halfmoveClock = 0;
    fullmoveNumber = 1;

    updateZobristKey();
}

void Board::print() const
{
    (void)0;
}

void Board::movePiece(Piece piece, Square from, Square to)
{
    removePiece(piece, from);
    setPiece(piece, to);
}

void Board::makeMove(
    const Move& move,
    UndoInfo& undoInfo)
{
    //--------------------------------------------------
    // Save current board state
    //--------------------------------------------------

    if (move.flag == MoveFlag::EnPassant)
    {
        Square capturedPawn =
            (move.piece == Piece::WhitePawn)
            ? static_cast<Square>(static_cast<int>(move.to) - 8)
            : static_cast<Square>(static_cast<int>(move.to) + 8);

        undoInfo.capturedPiece = pieceAt(capturedPawn);
    }
    else
    {
        undoInfo.capturedPiece = move.capturedPiece;
    }

undoInfo.castlingRights = castlingRights;
    undoInfo.enPassantSquare = enPassantSquare;
    undoInfo.sideToMove = sideToMove;
    undoInfo.halfmoveClock = halfmoveClock;
    undoInfo.fullmoveNumber = fullmoveNumber;

    //--------------------------------------------------
    // Clear en passant square
    //--------------------------------------------------

    enPassantSquare = Square::None;

    //--------------------------------------------------
    // Execute move
    //--------------------------------------------------

    switch (move.flag)
    {
    //--------------------------------------------------
    // Normal move
    //--------------------------------------------------

    case MoveFlag::Quiet:
    {
        removePieceInternal(move.piece, move.from);
        setPieceInternal(move.piece, move.to);

        break;
    }

    //--------------------------------------------------
    // Capture
    //--------------------------------------------------

    case MoveFlag::Capture:
    {
        removePieceInternal(undoInfo.capturedPiece, move.to);
        removePieceInternal(move.piece, move.from);
        setPieceInternal(move.piece, move.to);

        break;
    }

    //--------------------------------------------------
    // Double pawn push
    //--------------------------------------------------

    case MoveFlag::DoublePawnPush:
{
    removePieceInternal(move.piece, move.from);
    setPieceInternal(move.piece, move.to);

    if (move.piece == Piece::WhitePawn)
    {
        enPassantSquare = Square(
            static_cast<int>(move.from) + 8);
    }
    else
    {
        enPassantSquare = Square(
            static_cast<int>(move.from) - 8);
    }

    break;
}

    //--------------------------------------------------
    // King side castle
    //--------------------------------------------------

    case MoveFlag::KingCastle:
{
    removePieceInternal(move.piece, move.from);
    setPieceInternal(move.piece, move.to);

    if (move.piece == Piece::WhiteKing)
    {
        removePieceInternal(Piece::WhiteRook, Square::H1);
        setPieceInternal(Piece::WhiteRook, Square::F1);
    }
    else
    {
        removePieceInternal(Piece::BlackRook, Square::H8);
        setPieceInternal(Piece::BlackRook, Square::F8);
    }

    break;
}

    //--------------------------------------------------
    // Queen side castle
    //--------------------------------------------------

    case MoveFlag::QueenCastle:
{
    removePieceInternal(move.piece, move.from);
    setPieceInternal(move.piece, move.to);

    if (move.piece == Piece::WhiteKing)
    {
        removePieceInternal(Piece::WhiteRook, Square::A1);
        setPieceInternal(Piece::WhiteRook, Square::D1);
    }
    else
    {
        removePieceInternal(Piece::BlackRook, Square::A8);
        setPieceInternal(Piece::BlackRook, Square::D8);
    }

    break;
}

    //--------------------------------------------------
    // En passant
    //--------------------------------------------------

    case MoveFlag::EnPassant:
{
    removePieceInternal(move.piece, move.from);
    setPieceInternal(move.piece, move.to);

    Square capturedPawn =
        (move.piece == Piece::WhitePawn)
        ? static_cast<Square>(static_cast<int>(move.to) - 8)
        : static_cast<Square>(static_cast<int>(move.to) + 8);

    removePieceInternal(undoInfo.capturedPiece, capturedPawn);

    break;
}

    //--------------------------------------------------
    // Promotions
    //--------------------------------------------------

case MoveFlag::PromotionKnight:
case MoveFlag::PromotionBishop:
case MoveFlag::PromotionRook:
case MoveFlag::PromotionQueen:

case MoveFlag::PromotionCaptureKnight:
case MoveFlag::PromotionCaptureBishop:
case MoveFlag::PromotionCaptureRook:
case MoveFlag::PromotionCaptureQueen:
{
    Piece promotedPiece;

    if (move.piece == Piece::WhitePawn)
    {
        switch (move.flag)
        {
        case MoveFlag::PromotionKnight:
            promotedPiece = Piece::WhiteKnight;
            break;

        case MoveFlag::PromotionBishop:
            promotedPiece = Piece::WhiteBishop;
            break;

        case MoveFlag::PromotionRook:
            promotedPiece = Piece::WhiteRook;
            break;

        default:
            promotedPiece = Piece::WhiteQueen;
            break;
        }
    }
    else
    {
        switch (move.flag)
        {
        case MoveFlag::PromotionKnight:
            promotedPiece = Piece::BlackKnight;
            break;

        case MoveFlag::PromotionBishop:
            promotedPiece = Piece::BlackBishop;
            break;

        case MoveFlag::PromotionRook:
            promotedPiece = Piece::BlackRook;
            break;

        default:
            promotedPiece = Piece::BlackQueen;
            break;
        }
    }

    if (move.capturedPiece != Piece::None)
    {
        removePieceInternal(
            move.capturedPiece,
            move.to);
    }

    removePieceInternal(
        move.piece,
        move.from);

    setPieceInternal(
        promotedPiece,
        move.to);
}
    }
    
    //--------------------------------------------------
    // Update castling rights
    //--------------------------------------------------

    switch (move.from)
    {
    case Square::E1:
        castlingRights &= ~(0b0001 | 0b0010);
        break;

    case Square::H1:
        castlingRights &= ~0b0001;
        break;

    case Square::A1:
        castlingRights &= ~0b0010;
        break;

    case Square::E8:
        castlingRights &= ~(0b0100 | 0b1000);
        break;

    case Square::H8:
        castlingRights &= ~0b0100;
        break;

    case Square::A8:
        castlingRights &= ~0b1000;
        break;

    default:
        break;
}

    //--------------------------------------------------
    // Captured rook loses castling rights
    //--------------------------------------------------

    switch (move.to)
    {
    case Square::H1:
        castlingRights &= ~0b0001;
        break;

    case Square::A1:
        castlingRights &= ~0b0010;
        break;

    case Square::H8:
        castlingRights &= ~0b0100;
        break;

    case Square::A8:
        castlingRights &= ~0b1000;
        break;

    default:
        break;
}

    //--------------------------------------------------
    // Update halfmove clock
    //--------------------------------------------------

    if (move.piece == Piece::WhitePawn ||
        move.piece == Piece::BlackPawn ||
        undoInfo.capturedPiece != Piece::None)
    {
        halfmoveClock = 0;
    }
    else
    {
        halfmoveClock++;
    }

    //--------------------------------------------------
    // Change side
    //--------------------------------------------------

    if (sideToMove == ChessColor::Black)
    {
        fullmoveNumber++;
    }

    sideToMove =
        (sideToMove == ChessColor::White)
        ? ChessColor::Black
        : ChessColor::White;

    clearPieceAttacks(move.from);
    clearPieceAttacks(move.to);
    clearPieceMoves(move.from);
    clearPieceMoves(move.to);

    updateZobristKey();
    updateOccupancy();
}

void Board::undoMove(
    const Move& move,
    const UndoInfo& undoInfo)
{
    //--------------------------------------------------
    // Restore board state
    //--------------------------------------------------

sideToMove = undoInfo.sideToMove;
    castlingRights = undoInfo.castlingRights;
    enPassantSquare = undoInfo.enPassantSquare;
    halfmoveClock = undoInfo.halfmoveClock;
    fullmoveNumber = undoInfo.fullmoveNumber;

    //--------------------------------------------------
    // Undo move
    //--------------------------------------------------

    switch (move.flag)
    {
    //--------------------------------------------------
    // Normal move
    //--------------------------------------------------

    case MoveFlag::Quiet:
    {
        removePieceInternal(
            move.piece,
            move.to);
        setPieceInternal(
            move.piece,
            move.from);

        break;
    }

    //--------------------------------------------------
    // Capture
    //--------------------------------------------------

    case MoveFlag::Capture:
    {
        removePieceInternal(
            move.piece,
            move.to);
        setPieceInternal(
            move.piece,
            move.from);

        setPieceInternal(
            undoInfo.capturedPiece,
            move.to);

        break;
    }

    //--------------------------------------------------
    // Double pawn push
    //--------------------------------------------------

    case MoveFlag::DoublePawnPush:
    {
        removePieceInternal(
            move.piece,
            move.to);
        setPieceInternal(
            move.piece,
            move.from);

        break;
    }

    //--------------------------------------------------
    // King side castle
    //--------------------------------------------------

    case MoveFlag::KingCastle:
{
    removePieceInternal(
        move.piece,
        move.to);
    setPieceInternal(
        move.piece,
        move.from);

    if (move.piece == Piece::WhiteKing)
    {
        removePieceInternal(
            Piece::WhiteRook,
            Square::F1);
        setPieceInternal(
            Piece::WhiteRook,
            Square::H1);
    }
    else
    {
        removePieceInternal(
            Piece::BlackRook,
            Square::F8);
        setPieceInternal(
            Piece::BlackRook,
            Square::H8);
    }

    break;
}

    //--------------------------------------------------
    // Queen side castle
    //--------------------------------------------------

    case MoveFlag::QueenCastle:
{
    removePieceInternal(
        move.piece,
        move.to);
    setPieceInternal(
        move.piece,
        move.from);

    if (move.piece == Piece::WhiteKing)
    {
        removePieceInternal(
            Piece::WhiteRook,
            Square::D1);
        setPieceInternal(
            Piece::WhiteRook,
            Square::A1);
    }
    else
    {
        removePieceInternal(
            Piece::BlackRook,
            Square::D8);
        setPieceInternal(
            Piece::BlackRook,
            Square::A8);
    }

    break;
}

    //--------------------------------------------------
    // En passant
    //--------------------------------------------------

    case MoveFlag::EnPassant:
    {
        removePieceInternal(
            move.piece,
            move.to);
        setPieceInternal(
            move.piece,
            move.from);

        Square capturedPawn =
            (move.piece == Piece::WhitePawn)
            ? static_cast<Square>(static_cast<int>(move.to) - 8)
            : static_cast<Square>(static_cast<int>(move.to) + 8);

        setPieceInternal(
            undoInfo.capturedPiece,
            capturedPawn);

        break;
    }

    //--------------------------------------------------
    // Promotions
    //--------------------------------------------------

    case MoveFlag::PromotionKnight:
    case MoveFlag::PromotionBishop:
    case MoveFlag::PromotionRook:
    case MoveFlag::PromotionQueen:

    case MoveFlag::PromotionCaptureKnight:
    case MoveFlag::PromotionCaptureBishop:
    case MoveFlag::PromotionCaptureRook:
    case MoveFlag::PromotionCaptureQueen:
    {
        Piece promotedPiece;

        if (move.piece == Piece::WhitePawn)
        {
            switch (move.flag)
            {
            case MoveFlag::PromotionKnight:
                promotedPiece = Piece::WhiteKnight;
                break;

            case MoveFlag::PromotionBishop:
                promotedPiece = Piece::WhiteBishop;
                break;

            case MoveFlag::PromotionRook:
                promotedPiece = Piece::WhiteRook;
                break;

            default:
                promotedPiece = Piece::WhiteQueen;
                break;
            }
        }
        else
        {
            switch (move.flag)
            {
            case MoveFlag::PromotionKnight:
                promotedPiece = Piece::BlackKnight;
                break;

            case MoveFlag::PromotionBishop:
                promotedPiece = Piece::BlackBishop;
                break;

            case MoveFlag::PromotionRook:
                promotedPiece = Piece::BlackRook;
                break;

            default:
                promotedPiece = Piece::BlackQueen;
                break;
            }
        }

        removePieceInternal(
            promotedPiece,
            move.to);

        setPieceInternal(
            move.piece,
            move.from);

        if (undoInfo.capturedPiece != Piece::None)
        {
            setPieceInternal(
                undoInfo.capturedPiece,
                move.to);
        }

        break;
        }
    }

    //--------------------------------------------------
    // Update occupancies
    //--------------------------------------------------

    clearPieceAttacks(move.from);
    clearPieceAttacks(move.to);
    clearPieceMoves(move.from);
    clearPieceMoves(move.to);

    updateZobristKey();

    updateOccupancy();
}

void Board::makeNullMove(
    UndoInfo& undoInfo)
{
    undoInfo.castlingRights = castlingRights;
    undoInfo.enPassantSquare = enPassantSquare;
    undoInfo.sideToMove = sideToMove;
undoInfo.halfmoveClock = halfmoveClock;
    undoInfo.wasNullMove = true;

    enPassantSquare = Square::None;

    sideToMove =
        (sideToMove == ChessColor::White)
        ? ChessColor::Black
        : ChessColor::White;

    updateZobristKey();
}

void Board::undoNullMove(
    const UndoInfo& undoInfo)
{
    castlingRights = undoInfo.castlingRights;
    enPassantSquare = undoInfo.enPassantSquare;
    sideToMove = undoInfo.sideToMove;
halfmoveClock = undoInfo.halfmoveClock;

    updateZobristKey();
}

Piece Board::pieceAt(Square square) const
{
    if (square == Square::None)
    {
        return Piece::None;
    }
    
    return mailbox[static_cast<int>(square)];
}

void Board::updateZobristKey()
{
    zobristKey = Zobrist::calculateHash(*this);
}

void Board::updateOccupancy()
{
    whiteOccupancy = 0;
    blackOccupancy = 0;

    for (int i = static_cast<int>(Piece::WhitePawn);
         i <= static_cast<int>(Piece::WhiteKing);
         i++)
    {
        whiteOccupancy |= bitboards[i];
    }

    for (int i = static_cast<int>(Piece::BlackPawn);
         i <= static_cast<int>(Piece::BlackKing);
         i++)
    {
        blackOccupancy |= bitboards[i];
    }

    allOccupancy = whiteOccupancy | blackOccupancy;
}

Bitboard Board::getBitboard(Piece piece) const
{
    return bitboards[static_cast<int>(piece)];
}

Bitboard Board::getOccupancy(ChessColor color) const
{
    return (color == ChessColor::White)
        ? whiteOccupancy
        : blackOccupancy;
}

Bitboard Board::getAllOccupancy() const
{
    return allOccupancy;
}

ChessColor Board::getSideToMove() const
{
    return sideToMove;
}

Square Board::getEnPassantSquare() const
{
    return enPassantSquare;
}

int Board::getHalfmoveClock() const
{
    return halfmoveClock;
}

uint8_t Board::getCastlingRights() const
{
    return castlingRights;
}

uint64_t Board::getZobristKey() const
{
    return zobristKey;
}

int Board::getFullmoveNumber() const
{
    return fullmoveNumber;
}

Square Board::getKingSquare(
    ChessColor color) const
{
    Bitboard kingBoard =
        getBitboard(
            color == ChessColor::White
            ? Piece::WhiteKing
            : Piece::BlackKing);

    if (kingBoard == 0)
    {
        return Square::None;
    }

    return static_cast<Square>(
        std::countr_zero(kingBoard));
}

Bitboard Board::getPieceAttacks(Square square) const
{
    return attackBitboards[static_cast<int>(square)];
}

void Board::setPieceAttacks(Square square, Bitboard attacks)
{
    attackBitboards[static_cast<int>(square)] = attacks;
}

void Board::clearPieceAttacks(Square square)
{
    attackBitboards[static_cast<int>(square)] = 0;
}

Bitboard Board::getPieceMoves(Square square) const
{
    return moveBitboards[static_cast<int>(square)];
}

void Board::setPieceMoves(Square square, Bitboard moves)
{
    moveBitboards[static_cast<int>(square)] = moves;
}

void Board::clearPieceMoves(Square square)
{
    moveBitboards[static_cast<int>(square)] = 0;
}

void Board::clearAllPieceBitboards()
{
    attackBitboards.fill(0);
    moveBitboards.fill(0);
}
