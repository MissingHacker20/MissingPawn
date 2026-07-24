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
        const std::string& value);

    static const std::vector<UCIOption>& getOptions();

    static bool getBoolOption(
        const std::string& name);

private:

    static std::vector<UCIOption> options;
};