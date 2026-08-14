#pragma once

#include <string>
#include <vector>
#include <thread>
#include <atomic>

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

    // Wątek przeszukujący - pozwala na przerywanie searchu (komenda
    // "stop") bez blokowania głównej pętli UCI.
    static std::thread searchThread;
    static std::atomic<bool> searchActive;

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

    static bool debugMode;
};