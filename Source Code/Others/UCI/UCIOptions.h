/*
 * MissingPawn
 * Copyright (C) 2026 Kacper Wieczorek
 *
 * This file is part of MissingPawn.
 *
 * Licensed under the GNU General Public License v3 or later.
 * See LICENSE for details.
 */

 #pragma once

#include <vector>

#include "UCIOptionData.h"

class UCIOptions
{
public:

    static int getIntOption(
        const std::string& name);

    static void initialize();

    static bool setOption(
        const std::string& name,
        const std::string& value,
        std::string& errorMessage);

    static const std::vector<UCIOption>& getOptions();

    static bool getBoolOption(
        const std::string& name);

private:

    static std::vector<UCIOption> options;
};