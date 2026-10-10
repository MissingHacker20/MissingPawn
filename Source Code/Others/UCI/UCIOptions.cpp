/*
 * MissingPawn
 * Copyright (C) 2026 Kacper Wieczorek
 *
 * This file is part of MissingPawn.
 *
 * Licensed under the GNU General Public License v3 or later.
 * See LICENSE for details.
 */

 #include "Others/UCI/UCIOptions.h"

#include <vector>
#include <algorithm>
#include <charconv>

std::vector<UCIOption> UCIOptions::options;

void UCIOptions::initialize()
{
    options.clear();

    options.push_back(
    {
        "Hash",
        UCIOptionType::Spin,
        "64",
        "64",
        1,
        4096
    });

    options.push_back(
    {
        "Ponder",
        UCIOptionType::Check,
        "false",
        "false"
    });

    options.push_back(
    {
        "BookMoves",
        UCIOptionType::Check,
        "false",
        "false"
    });
}

bool UCIOptions::setOption(
    const std::string& name,
    const std::string& value,
    std::string& errorMessage)
{
    for (auto& option : options)
    {
        if (option.name != name)
            continue;

        if (option.type == UCIOptionType::Check)
        {
            if (value != "true" && value != "false")
            {
                errorMessage = "Option '" + name + "' expects true or false.";
                return false;
            }
        }
        else if (option.type == UCIOptionType::Spin)
        {
            int parsed = 0;
            const char* begin = value.data();
            const char* end = begin + value.size();
            const auto [ptr, ec] = std::from_chars(begin, end, parsed);
            if (ec != std::errc{} || ptr != end || parsed < option.min || parsed > option.max)
            {
                errorMessage = "Option '" + name + "' must be an integer from " +
                    std::to_string(option.min) + " to " + std::to_string(option.max) + ".";
                return false;
            }
        }

        option.currentValue = value;
        return true;
    }

    errorMessage = "Unknown option: " + name;
    return false;
}

const std::vector<UCIOption>& UCIOptions::getOptions()
{
    return options;
}

int UCIOptions::getIntOption(
    const std::string& name)
{
    for (const auto& option : options)
    {
        if (option.name == name)
        {
            return std::stoi(
                option.currentValue);
        }
    }

    return 0;
}

bool UCIOptions::getBoolOption(
    const std::string& name)
{
    for (const auto& option : options)
    {
        if (option.name == name)
        {
            return option.currentValue == "true";
        }
    }

    return false;
}
