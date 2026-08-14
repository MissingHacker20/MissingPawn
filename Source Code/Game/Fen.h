#pragma once

#include <string>

class Board;

class Fen
{
public:
    static bool load(Board& board, const std::string& fen);
};
