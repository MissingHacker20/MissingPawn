#include "KillerMoves.h"

Move KillerMoves::killers
    [MaxDepth]
    [KillerCount];

void KillerMoves::clear()
{
    for (int d = 0; d < MaxDepth; d++)
    {
        for (int i = 0; i < KillerCount; i++)
        {
            killers[d][i] = Move{};
        }
    }
}

void KillerMoves::add(
    int depth,
    const Move& move)
{
    if (depth >= MaxDepth)
    {
        return;
    }

    //--------------------------------------------------
    // Już zapisany
    //--------------------------------------------------

    for (int i = 0; i < KillerCount; i++)
    {
        if (killers[depth][i] == move)
        {
            return;
        }
    }

    //--------------------------------------------------
    // Przesunięcie
    //--------------------------------------------------

    for (int i = KillerCount - 1; i > 0; i--)
    {
        killers[depth][i] =
            killers[depth][i - 1];
    }

    killers[depth][0] = move;
}

int KillerMoves::score(
    int depth,
    const Move& move)
{
    if (depth >= MaxDepth)
    {
        return 0;
    }

    if (killers[depth][0] == move)
    {
        return 50000;
    }

    if (killers[depth][1] == move)
    {
        return 40000;
    }

    return 0;
}