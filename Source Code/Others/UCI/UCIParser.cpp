/*
 * MissingPawn
 * Copyright (C) 2026 Kacper Wieczorek
 *
 * This file is part of MissingPawn.
 *
 * Licensed under the GNU General Public License v3 or later.
 * See LICENSE for details.
 */

 #include "Others/UCI/UCIParser.h"

#include <charconv>

GoParameters UCIParser::parseGo(
    const std::vector<std::string>& tokens,
    std::string& errorMessage)
{
    errorMessage.clear();
    GoParameters params;
    bool hasDepth = false;

    for (size_t i = 0; i < tokens.size(); i++)
    {
        const auto parseInt = [&](int& value) {
            if (i + 1 >= tokens.size())
            {
                errorMessage = "Missing value for '" + tokens[i] + "'.";
                return false;
            }
            const std::string& text = tokens[++i];
            const char* begin = text.data();
            const char* end = begin + text.size();
            const auto [ptr, error] = std::from_chars(begin, end, value);
            if (error != std::errc{} || ptr != end || value < 0)
            {
                errorMessage = "Invalid non-negative integer for '" + tokens[i - 1] + "': " + text;
                return false;
            }
            return true;
        };
        const auto parseUnsigned = [&](uint64_t& value) {
            if (i + 1 >= tokens.size())
            {
                errorMessage = "Missing value for 'nodes'.";
                return false;
            }
            const std::string& text = tokens[++i];
            const char* begin = text.data();
            const char* end = begin + text.size();
            const auto [ptr, error] = std::from_chars(begin, end, value);
            if (error != std::errc{} || ptr != end)
            {
                errorMessage = "Invalid non-negative integer for 'nodes': " + text;
                return false;
            }
            return true;
        };

        if (i == 0 && tokens[i] == "go")
            continue;
        if (tokens[i] == "depth")
        {
            hasDepth = true;
            if (!parseInt(params.depth)) return {};
        }
        else if (tokens[i] == "movetime")
        {
            if (!parseInt(params.moveTime)) return {};
            params.hasMoveTime = true;
        }
        else if (tokens[i] == "wtime")
        {
            if (!parseInt(params.whiteTime)) return {};
        }
        else if (tokens[i] == "btime")
        {
            if (!parseInt(params.blackTime)) return {};
        }
        else if (tokens[i] == "winc")
        {
            if (!parseInt(params.whiteIncrement)) return {};
        }
        else if (tokens[i] == "binc")
        {
            if (!parseInt(params.blackIncrement)) return {};
        }
        else if (tokens[i] == "movestogo")
        {
            if (!parseInt(params.movesToGo)) return {};
        }
        else if (tokens[i] == "nodes")
        {
            if (!parseUnsigned(params.nodeLimit)) return {};
        }
        else if (tokens[i] == "mate")
        {
            if (!parseInt(params.mateDepth)) return {};
        }
        else if (tokens[i] == "searchmoves")
        {
            errorMessage = "The 'searchmoves' option is not currently supported.";
            return {};
        }
        else if (tokens[i] == "ponder")
            params.ponder = true;
        else if (tokens[i] == "infinite")
            params.infinite = true;
        else
        {
            errorMessage = "Unknown 'go' parameter: " + tokens[i];
            return {};
        }
    }

    if (hasDepth && params.depth == 0)
    {
        errorMessage = "Search depth must be at least 1.";
        return {};
    }
    if (params.movesToGo == 0)
    {
        errorMessage = "movestogo must be at least 1.";
        return {};
    }
    return params;
}
