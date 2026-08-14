#pragma once

#include <string>

enum class UCIOptionType
{
    Check,
    Spin,
    Combo,
    Button,
    String
};

struct UCIOption
{
    std::string name;

    UCIOptionType type;

    std::string defaultValue;

    std::string currentValue;

    int min = 0;

    int max = 0;
};