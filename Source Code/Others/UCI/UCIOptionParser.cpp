/*
 * MissingPawn
 * Copyright (C) 2026 Kacper Wieczorek
 *
 * This file is part of MissingPawn.
 *
 * Licensed under the GNU General Public License v3 or later.
 * See LICENSE for details.
 */

 #include "Others/UCI/UCIOptionParser.h"

#include "Others/UCI/UCIOptions.h"
#include "Engine/Search.h"

bool UCIOptionParser::parse(
    const std::vector<std::string>& tokens,
    std::string& errorMessage)
{
    errorMessage.clear();
    if (tokens.size() < 2 || tokens[1] != "name")
    {
        errorMessage = "Expected 'setoption name <name> value <value>'.";
        return false;
    }
    std::string name;
    std::string value;

    bool readName = false;
    bool readValue = false;

    for (size_t i = 1; i < tokens.size(); i++)
    {
        if (tokens[i] == "name")
        {
            readName = true;
            readValue = false;
            continue;
        }

        if (tokens[i] == "value")
        {
            readName = false;
            readValue = true;
            continue;
        }

        if (readName)
        {
            if (!name.empty())
            {
                name += " ";
            }

            name += tokens[i];
        }

        if (readValue)
        {
            if (!value.empty())
            {
                value += " ";
            }

            value += tokens[i];
        }
    }

    if (name.empty())
    {
        errorMessage = "Option name is empty.";
        return false;
    }
    if (!readValue)
    {
        errorMessage = "Missing 'value' field for option '" + name + "'.";
        return false;
    }

    if (!UCIOptions::setOption(name, value, errorMessage))
        return false;

    if (name == "Ponder")
        Search::setPonder(UCIOptions::getBoolOption("Ponder"));

    return true;
}
