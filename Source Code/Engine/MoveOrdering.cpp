#include "MoveOrdering.h"
#include "KillerMoves.h"
#include "HistoryHeuristic.h"
#include "Move/MoveValidator.h"

#include <algorithm>
#include <cstdlib>

constexpr int PieceValue[]
{
    0,

    100,   // WhitePawn
    320,   // WhiteKnight
    330,   // WhiteBishop
    500,   // WhiteRook
    900,   // WhiteQueen
    20000, // WhiteKing

    100,   // BlackPawn
    320,   // BlackKnight
    330,   // BlackBishop
    500,   // BlackRook
    900,   // BlackQueen
    20000  // BlackKing
};

// Ładuje stosunkową wartość bierki niezależnie od koloru
inline int valueOfPiece(Piece piece)
{
    if (piece == Piece::None)
    {
        return 0;
    }

    if (piece >= Piece::WhitePawn && piece <= Piece::WhiteKing)
    {
        return PieceValue[static_cast<int>(piece)];
    }

    return PieceValue[static_cast<int>(piece)];
}

bool MoveOrdering::isCapture(const Move& move)
{
    return move.capturedPiece != Piece::None;
}

bool MoveOrdering::isPromotion(const Move& move)
{
    return move.flag >= MoveFlag::PromotionKnight &&
           move.flag <= MoveFlag::PromotionCaptureQueen;
}

int MoveOrdering::scoreMove(
    Board& board,
    const Move& move,
    int depth,
    int ply,
    const Move& ttMove,
    int cachedTactical)
{
    (void)depth;
    int score = 0;

    //--------------------------------------------------------
    // 1) Principal Variation (TT) move - zawsze na górze
    //--------------------------------------------------------

    if (ttMove.from != Square::None && move == ttMove)
    {
        return 2000000;
    }

    //--------------------------------------------------------
    // 2) Bicia: MVV-LVA
    //--------------------------------------------------------

    if (isCapture(move))
    {
        const int victim = valueOfPiece(move.capturedPiece);
        const int attacker = valueOfPiece(move.piece);

        // Nagroda za bicie - dużo wyżej niż zwykły ruch
        score += 1000000;

        // MVV-LVA: wyżej bicia wartościowych bierek przez mniej wartościowe
        score += victim * 100 - attacker;

        // Static Exchange Evaluation: zyskowna wymiana (SEE > 0) ląduje
        // wyżej niż równa (0), a stratna (SEE < 0) niżej - dzięki temu
        // silnik najpierw próbuje bić figury, które po pełnej wymianie
        // faktycznie zostają zyskiem (np. goniec bierze hetmana, który
        // nie zostanie odzyskany), zamiast tych, po których sam ginie.
        const int seeScore = MoveValidator::see(board, move.to);
        score += seeScore;

        // Promocja z biciem jeszcze wyżej
        if (isPromotion(move))
        {
            score += 100000;
        }

        return score;
    }

    //--------------------------------------------------------
    // 3) Promocje (bez bicia)
    //--------------------------------------------------------

    if (isPromotion(move))
    {
        switch (move.flag)
        {
        case MoveFlag::PromotionQueen: score += 900000; break;
        case MoveFlag::PromotionRook:  score += 800000; break;
        case MoveFlag::PromotionKnight: score += 700000; break;
        case MoveFlag::PromotionBishop: score += 600000; break;
        default: score += 500000; break;
        }

        return score;
    }

    //--------------------------------------------------------
    // 4) Killer moves (per-ply) - wyżej niż zwykłe ciche ruchy z historią
    //--------------------------------------------------------

    score += KillerMoves::score(ply, move);

    //--------------------------------------------------------
    // 5) Historia (nie powinna dominować nad killerami)
    //--------------------------------------------------------

    const int historyScore = HistoryHeuristic::get(
        board.getSideToMove(),
        move);

    score += std::min(historyScore, 20000);

    //--------------------------------------------------------
    // 6) Preferuj rozwój, centralizację i małe zachęty
    //--------------------------------------------------------

    if (move.flag == MoveFlag::KingCastle ||
        move.flag == MoveFlag::QueenCastle)
    {
        score += 50;
    }

    //--------------------------------------------------------
    // Nie używamy cachedTactical do oceny pojedynczego ruchu. Jest to wynik
    // pozycji rodzica, a nie pozycji po wykonaniu ruchu, więc wspólna kara
    // mogłaby odwracać kolejność dobrych ruchów. Taktykę ruchu ocenia SEE
    // dla bić; ciche ruchy pozostają oceniane przez historię i killer moves.
    (void)cachedTactical;
    return score;
}

void MoveOrdering::sortMoves(
    Board& board,
    MoveList& moves,
    int depth,
    int ply,
    const Move& ttMove,
    int cachedTactical)
{
    const int count = moves.size();

    // Wstępnie policz wynik dla KAŻDEGO ruchu dokładnie raz.
    // Wcześniej scoreMove był wołany wielokrotnie w każdym porównaniu
    // std::sort (O(n log n) razy), co było kosztowne. Teraz sortujemy
    // po indeksach, a wynik każdego ruchu jest stały - kolejność po
    // sortowaniu jest identyczna jak wcześniej, ale dużo szybsza.
    //
    // Używamy stałej tablicy na stosie (zamiast std::vector), co całkowicie
    // eliminuje alokacje heap WHILE wyszukiwania. Maksymalna liczba legalnych
    // ruchów w szachach to 218, więc 256 jest bezpiecznym górnym limitem.
    constexpr int MaxMoves = 256;
    int scores[MaxMoves];
    int order[MaxMoves];

    for (int i = 0; i < count; ++i)
    {
        scores[i] = scoreMove(board, moves[i], depth, ply, ttMove, cachedTactical);
        order[i] = i;
    }

    // Sortujemy tablicę indeksów po prekomputowanych wynikach (malejąco).
    std::sort(order, order + count, [&](int a, int b)
    {
        return scores[a] > scores[b];
    });

    // Przepisz ruchy w nowej (posortowanej) kolejności, na miejscu.
    Move scratch[MaxMoves];
    for (int i = 0; i < count; ++i)
    {
        scratch[i] = moves[order[i]];
    }
    for (int i = 0; i < count; ++i)
    {
        moves[i] = scratch[i];
    }
}

