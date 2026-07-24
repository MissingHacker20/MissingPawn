#include "Others/UCI/UCIOptionParser.h"

#include "Others/UCI/UCIOptions.h"
#include "Engine/Transposition.h"
#include "Engine/Search.h"

void UCIOptionParser::parse(
    const std::vector<std::string>& tokens)
{
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

    if (!UCIOptions::setOption(name, value))
    {
        return;
    }

    //--------------------------------------------------
    // Engine reaction
    //--------------------------------------------------

    if (name == "Hash")
    {
        TranspositionTable::resize(
            static_cast<size_t>(
                UCIOptions::getIntOption("Hash")));
    }

    else if (name == "Threads")
    {
        // Single-threaded engine - ignore Threads setting for now
        // Future: implement multi-threading
        std::cout << "info string Missing Pawn is single-threaded. Threads option ignored.\n";
    }

    else if (name == "Ponder")
    {
        Search::setPonder(
            UCIOptions::getBoolOption("Ponder"));
    }
}
