#include "Others/UCI/UCIParser.h"

GoParameters UCIParser::parseGo(
    const std::vector<std::string>& tokens)
{
    GoParameters params;

    for (size_t i = 0; i < tokens.size(); i++)
    {
        try
        {
            if (tokens[i] == "depth" && i + 1 < tokens.size())
            {
                params.depth =
                    std::stoi(tokens[i + 1]);
            }

            else if (tokens[i] == "movetime" && i + 1 < tokens.size())
            {
                params.moveTime =
                    std::stoi(tokens[i + 1]);
            }

            else if (tokens[i] == "wtime" && i + 1 < tokens.size())
            {
                params.whiteTime =
                    std::stoi(tokens[i + 1]);
            }

            else if (tokens[i] == "btime" && i + 1 < tokens.size())
            {
                params.blackTime =
                    std::stoi(tokens[i + 1]);
            }

            else if (tokens[i] == "winc" && i + 1 < tokens.size())
            {
                params.whiteIncrement =
                    std::stoi(tokens[i + 1]);
            }

            else if (tokens[i] == "binc" && i + 1 < tokens.size())
            {
                params.blackIncrement =
                    std::stoi(tokens[i + 1]);
            }

            else if (tokens[i] == "movestogo" && i + 1 < tokens.size())
            {
                params.movesToGo =
                    std::stoi(tokens[i + 1]);
            }

            else if (tokens[i] == "nodes" && i + 1 < tokens.size())
            {
                params.nodeLimit =
                    std::stoull(tokens[i + 1]);
            }

            else if (tokens[i] == "mate" && i + 1 < tokens.size())
            {
                params.mateDepth =
                    std::stoi(tokens[i + 1]);
            }

            else if (tokens[i] == "ponder")
            {
                params.ponder = true;
            }

            else if (tokens[i] == "infinite")
            {
                params.infinite = true;
            }
        }
        catch (const std::exception&)
        {
            // Ignoruj nieprawidłowe wartości parametrów
            continue;
        }
    }

    return params;
}
