/*
 * MissingPawn
 * Copyright (C) 2026 Kacper Wieczorek
 *
 * This file is part of MissingPawn.
 *
 * Licensed under the GNU General Public License v3 or later.
 * See LICENSE for details.
 */

#include "Others/UCI/UCIPositionParser.h"
#include "Foundation/Board.h"
#include "Game/Fen.h"
#include "Game/GameHistory.h"
#include "Move/MoveParser.h"
#include "Move/MoveGenerator.h"
#include "Move/MoveList.h"
#include "Move/MoveValidator.h"

#include <algorithm>

bool UCIPositionParser::parse(
    Board& board,
    const std::vector<std::string>& tokens,
    std::string& errorMessage)
{
    errorMessage.clear();
    if (tokens.size() < 2)
    {
        errorMessage = "Missing position arguments.";
        return false;
    }

    Board candidate = board;
    std::vector<uint64_t> historyKeys;

    //--------------------------------------------------
    // position startpos
    //--------------------------------------------------

    if (tokens[1] == "startpos")
    {
        if (tokens.size() > 2 && tokens[2] != "moves")
        {
            errorMessage = "Unexpected token after 'startpos': " + tokens[2];
            return false;
        }
        candidate.setStartPosition();

        historyKeys.push_back(candidate.getZobristKey());

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

                    if (!MoveParser::parseMove(candidate, tokens[j], move))
                    {
                        errorMessage = "Invalid move: " + tokens[j];
                        return false;
                    }

                    MoveList legalMoves;
                    MoveGenerator::generateMoves(candidate, legalMoves, MoveValidator::CheckInfo{});
                    const bool isLegal = std::find(legalMoves.begin(), legalMoves.end(), move) != legalMoves.end();
                    if (!isLegal)
                    {
                        errorMessage = "Illegal move: " + tokens[j];
                        return false;
                    }

                    UndoInfo undoInfo;
                    candidate.makeMove(move, undoInfo);
                    historyKeys.push_back(candidate.getZobristKey());
                }

                break;
            }
        }

        board = candidate;
        GameHistory::clear();
        for (const uint64_t key : historyKeys)
            GameHistory::pushPosition(key);
        return true;
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

        if (i - 2 != 6)
        {
            errorMessage = "FEN must contain exactly six fields.";
            return false;
        }
        if (!Fen::load(candidate, fen))
        {
            errorMessage = "Invalid FEN.";
            return false;
        }

        historyKeys.push_back(candidate.getZobristKey());

        //--------------------------------------------------
        // position fen ... moves ...
        //--------------------------------------------------

        if (i < tokens.size() && tokens[i] == "moves")
        {
            for (size_t j = i + 1; j < tokens.size(); j++)
            {
                Move move;

                if (!MoveParser::parseMove(candidate, tokens[j], move))
                {
                    errorMessage = "Invalid move: " + tokens[j];
                    return false;
                }

                MoveList legalMoves;
                MoveGenerator::generateMoves(candidate, legalMoves, MoveValidator::CheckInfo{});
                const bool isLegal = std::find(legalMoves.begin(), legalMoves.end(), move) != legalMoves.end();
                if (!isLegal)
                {
                    errorMessage = "Illegal move: " + tokens[j];
                    return false;
                }

                UndoInfo undoInfo;
                candidate.makeMove(move, undoInfo);
                historyKeys.push_back(candidate.getZobristKey());
            }
        }
        else if (i < tokens.size())
        {
            errorMessage = "Expected 'moves' after FEN fields.";
            return false;
        }

        board = candidate;
        GameHistory::clear();
        for (const uint64_t key : historyKeys)
            GameHistory::pushPosition(key);
        return true;
    }

    errorMessage = "Expected 'startpos' or 'fen'.";
    return false;
}
