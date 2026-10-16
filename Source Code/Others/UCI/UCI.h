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

    static uint64_t perft(Board& board, int depth);

private:

    // Wątek przeszukujący - pozwala na przerywanie searchu (komenda
    // "stop") bez blokowania głównej pętli UCI.
    static std::thread searchThread;
    static std::atomic<bool> searchActive;
    static std::atomic<bool> searchStarted;

    static Board board;

    static void commandUCI();

    static void commandIsReady();

    static void commandQuit();

    static void commandGo(
        const std::vector<std::string>& tokens);

    static void commandSaw();

    static void commandStop();

    static void commandUCINewGame();

    static std::vector<std::string> tokenize(
        const std::string& command);

    static void commandDebug(
    const std::vector<std::string>& tokens);

    static void commandBook(
        const std::vector<std::string>& tokens);

    static void commandRegister(
        const std::vector<std::string>& tokens);

static void commandPonderHit();

    static bool debugMode;
};
