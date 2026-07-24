#pragma once

#include <string>

enum class Square
{
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8,

    None,

    Count
};

inline Square stringToSquare(const std::string& text)
{
    if (text.size() != 2)
    {
        return Square::None;
    }

    char file = text[0];
    char rank = text[1];

    if (file < 'a' || file > 'h')
    {
        return Square::None;
    }

    if (rank < '1' || rank > '8')
    {
        return Square::None;
    }

    return static_cast<Square>(
        (rank - '1') * 8 +
        (file - 'a'));
}

inline std::string squareToString(Square square)
{
    if (square == Square::None)
    {
        return "--";
    }

    int index = static_cast<int>(square);

    char file = 'a' + (index % 8);
    char rank = '1' + (index / 8);

    return std::string{ file, rank };
}