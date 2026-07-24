#pragma once

#include <string>
#include <vector>

#include "Board.h"

class UCI
{
public:

    static void run();

    static bool executeCommand(
        const std::string& command);

    static bool shouldStop();

    static bool isDebugMode();

private:

    static Board board;

    static void commandUCI();

    static void commandIsReady();

    static void commandQuit();

    static void commandGo(
        const std::vector<std::string>& tokens);

    static void commandStop();

    static void commandUCINewGame();

    static std::vector<std::string> tokenize(
        const std::string& command);

    static void commandDebug(
    const std::vector<std::string>& tokens);

    static void commandRegister(
        const std::vector<std::string>& tokens);

    static void commandPonderHit();

    static void commandInfo();

    static bool debugMode;
};