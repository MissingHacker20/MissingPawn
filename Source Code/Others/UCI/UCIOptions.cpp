#include "Others/UCI/UCIOptions.h"

#include <vector>
#include <algorithm>

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
}

bool UCIOptions::setOption(
    const std::string& name,
    const std::string& value)
{
    for (auto& option : options)
    {
        if (option.name == name)
        {
            option.currentValue = value;
            return true;
        }
    }

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
