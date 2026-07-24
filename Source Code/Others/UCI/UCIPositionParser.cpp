#include "Others/UCI/UCIPositionParser.h"
#include "Foundation/Board.h"
#include "Game/Fen.h"
#include "Game/GameHistory.h"
#include "Move/MoveParser.h"

void UCIPositionParser::parse(
    Board& board,
    const std::vector<std::string>& tokens)
{
    if (tokens.size() < 2)
    {
        return;
    }

    //--------------------------------------------------
    // position startpos
    //--------------------------------------------------

    if (tokens[1] == "startpos")
    {
        board.setStartPosition();

        GameHistory::clear();

        GameHistory::pushPosition(
            board.getZobristKey());

        //--------------------------------------------------
        // position startpos moves ...
        //--------------------------------------------------

        for (size_t i = 2; i < tokens.size(); i++)
        {
            if (tokens[i] == "moves")
            {
                for (size_t j = i + 1; j < tokens.size(); j++)
                {
                    Move move;

                    if (!MoveParser::parseMove(
                            board,
                            tokens[j],
                            move))
                    {
                        return;
                    }

                    UndoInfo undoInfo;

                    board.makeMove(
                        move,
                        undoInfo);

                    GameHistory::pushPosition(
                    board.getZobristKey());
                }

                break;
            }
        }

        return;
    }

    //--------------------------------------------------
    // position fen ...
    //--------------------------------------------------

    if (tokens[1] == "fen")
    {
        std::string fen;

        size_t i = 2;

        // FEN składa się z 6 pól
        for (; i < tokens.size() && i < 8; i++)
        {
            if (!fen.empty())
            {
                fen += ' ';
            }

            fen += tokens[i];
        }

        if (!Fen::load(board, fen))
        {
            std::cout << "info string Invalid FEN\n";
            return;
        }

        GameHistory::clear();

        GameHistory::pushPosition(
            board.getZobristKey());

        //--------------------------------------------------
        // position fen ... moves ...
        //--------------------------------------------------

        if (i < tokens.size() && tokens[i] == "moves")
        {
            for (size_t j = i + 1; j < tokens.size(); j++)
            {
                Move move;

                if (!MoveParser::parseMove(
                        board,
                        tokens[j],
                        move))
                {
                    return;
                }

                UndoInfo undoInfo;

                board.makeMove(
                    move,
                    undoInfo);

                GameHistory::pushPosition(
                    board.getZobristKey());
            }
        }

        return;
    }
}
