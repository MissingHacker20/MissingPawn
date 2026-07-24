#pragma once

class Board;

enum class GameResult
{
    Playing,
    WhiteWin,
    BlackWin,
    Stalemate,
    FiftyMoveRule,
    ThreefoldRepetition
};

class GameState
{
public:
    static GameResult getResult(const Board& board);
};
