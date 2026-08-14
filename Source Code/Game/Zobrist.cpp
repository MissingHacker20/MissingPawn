#include "Game/Zobrist.h"

#include "Foundation/Board.h"
#include "Foundation/Piece.h"

namespace
{
// Prekomputowane tablice Zobrist
// zobristPiece[piece][square]
// piece: 0 = None, 1..12 = biały/czarny pion..król
uint64_t zobristPiece[13][64];
uint64_t zobristSide[2];
uint64_t zobristCastling[16];
uint64_t zobristEnPassant[64];

// Deterministyczny generator liczb pseudolosowych
// (SplitMix64) - te same wartości przy każdym uruchomieniu
uint64_t splitMix64(uint64_t& state)
{
    state += 0x9e3779b97f4a7c15ull;
    uint64_t z = state;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
    return z ^ (z >> 31);
}

bool initialized = false;

void initializeTables()
{
    if (initialized)
    {
        return;
    }

    uint64_t seed = 0x123456789abcdef0ull;

    for (int piece = 0; piece < 13; piece++)
    {
        for (int square = 0; square < 64; square++)
        {
            zobristPiece[piece][square] =
                splitMix64(seed);
        }
    }

    for (int i = 0; i < 2; i++)
    {
        zobristSide[i] = splitMix64(seed);
    }

    for (int i = 0; i < 16; i++)
    {
        zobristCastling[i] = splitMix64(seed);
    }

    for (int i = 0; i < 64; i++)
    {
        zobristEnPassant[i] = splitMix64(seed);
    }

    initialized = true;
}
}

namespace Zobrist
{
uint64_t calculateHash(const Board& board)
{
    initializeTables();

    uint64_t hash = 0;

    // Pionki i figury
    for (int square = 0; square < 64; ++square)
    {
        const Piece piece = board.pieceAt(static_cast<Square>(square));

        if (piece != Piece::None)
        {
            hash ^= zobristPiece[
                static_cast<int>(piece)][square];
        }
    }

    // Strona do ruchu
    if (board.getSideToMove() == ChessColor::Black)
    {
        hash ^= zobristSide[1];
    }

    // Prawa roszady
    hash ^= zobristCastling[board.getCastlingRights()];

    // En passant: wpływa na hash TYLKO gdy istnieje realna możliwość
    // wykonania bicia en passant. W przeciwnym razie pozycje różniące się
    // jedynie polem EP otrzymywałyby różne hasze, co psuje Transposition
    // Table (ta sama pozycja z "pustym" a "nieużywalnym" EP traktowana
    // byłaby jako inna).
    const Square ep = board.getEnPassantSquare();
    if (ep != Square::None)
    {
        const int epFile = static_cast<int>(ep) % 8;
        const int epRank = static_cast<int>(ep) / 8;

        // Strona do ruchu jest tą, która może wykonać en passant.
        const ChessColor side = board.getSideToMove();
        const Piece capturePawn =
            (side == ChessColor::White)
                ? Piece::WhitePawn
                : Piece::BlackPawn;

        // Pion bijący stoi na tej samej randze co pion, który wykonał
        // podwójny skok, czyli o jedną randze wyżej/niżej od pola EP.
        const int captureRank =
            (side == ChessColor::White)
                ? epRank - 1
                : epRank + 1;

        bool capturePossible = false;

        if (captureRank >= 0 && captureRank < 8)
        {
            for (int fileDelta : { -1, 1 })
            {
                const int captureFile = epFile + fileDelta;
                if (captureFile < 0 || captureFile >= 8)
                {
                    continue;
                }

                const Square captureSquare =
                    static_cast<Square>(captureRank * 8 + captureFile);

                if (board.pieceAt(captureSquare) == capturePawn)
                {
                    capturePossible = true;
                    break;
                }
            }
        }

        if (capturePossible)
        {
            hash ^= zobristEnPassant[static_cast<int>(ep)];
        }
    }

    return hash;
}
}
