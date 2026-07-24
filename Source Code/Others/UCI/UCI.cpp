#include "Others/UCI/UCI.h"
#include "Others/TimeManager.h"
#include "Others/UCI/UCIOptions.h"
#include "Engine/Transposition.h"
#include "Engine/Search.h"
#include "Engine/KillerMoves.h"
#include "Engine/HistoryHeuristic.h"
#include "Game/Fen.h"
#include "Others/UCI/UCIParser.h"
#include "Others/UCI/UCIPositionParser.h"
#include "Others/UCI/UCIOptionParser.h"

#include <iostream>
#include <sstream>

Board UCI::board;

bool UCI::debugMode = false;

bool UCI::executeCommand(
    const std::string& command)
{
    auto tokens = tokenize(command);

    if (tokens.empty())
    {
        return true;
    }

    if (tokens[0] == "uci")
    {
        commandUCI();
        return true;
    }

    if (tokens[0] == "isready")
    {
        commandIsReady();
        return true;
    }

    if (tokens[0] == "quit")
    {
        commandQuit();
        return false;
    }

    if (tokens[0] == "position")
    {
        UCIPositionParser::parse(
            board,
            tokens);
    }

    if (tokens[0] == "go")
    {
        commandGo(tokens);
        return true;
    }

    if (tokens[0] == "stop")
    {
        commandStop();
        return true;
    }

    if (tokens[0] == "ucinewgame")
    {
        commandUCINewGame();
        return true;
    }

    if (tokens[0] == "setoption")
    {
        UCIOptionParser::parse(tokens);
        return true;
    }

    if (tokens[0] == "debug")
    {
        commandDebug(tokens);
        return true;
    }

    if (tokens[0] == "ponderhit")
    {
        commandPonderHit();
        return true;
    }

    if (tokens[0] == "register")
    {
        commandRegister(tokens);
        return true;
    }

    return true;
}

void UCI::run()
{
    UCIOptions::initialize();

    std::string command;

    while (std::getline(
        std::cin,
        command))
    {
        if (!executeCommand(command))
        {
            break;
        }
    }
}

void UCI::commandUCI()
{
    std::cout << "id name Missing Pawn v1.0" << std::endl;
    std::cout << "id author Missing Player" << std::endl;
    std::cout << "option name Hash type spin default 64 min 1 max 4096" << std::endl;
    std::cout << "option name Threads type spin default 1 min 1 max 64" << std::endl;
    std::cout << "option name Ponder type check default false" << std::endl;
    std::cout << "uciok" << std::endl;
}

void UCI::commandIsReady()
{
    std::cout << "readyok" << std::endl;
}

void UCI::commandQuit()
{
    std::cout << "bye" << std::endl;
}

void UCI::commandGo(
    const std::vector<std::string>& tokens)
{
    GoParameters params =
    UCIParser::parseGo(tokens);

    int depth = params.depth > 0 ? params.depth : 6;

    TimeManager::reset();

    TimeManager::start();

    TimeManager::setDepth(params.depth);

    if (params.moveTime > 0)
    {
        TimeManager::setMoveTime(
            params.moveTime);
    }

    if (params.infinite)
    {
        TimeManager::setInfinite(true);
    }

    TimeManager::setWhiteTime(
        params.whiteTime);

    TimeManager::setBlackTime(
        params.blackTime);

    TimeManager::setWhiteIncrement(
        params.whiteIncrement);

    TimeManager::setBlackIncrement(
        params.blackIncrement);

    TimeManager::setMovesToGo(
        params.movesToGo);

    TimeManager::setNodeLimit(
        params.nodeLimit);

    TimeManager::setMateSearch(
        params.mateDepth);

    TimeManager::setPonder(
        params.ponder);

    Search::setPonder(
        params.ponder);

    // findBestMove already outputs "bestmove ..." internally
    Search::findBestMove(
        board,
        depth);
}

void UCI::commandStop()
{
    TimeManager::stop();
}

bool UCI::shouldStop()
{
    return TimeManager::shouldStop();
}

std::vector<std::string> UCI::tokenize(
    const std::string& command)
{
    std::stringstream ss(command);

    std::vector<std::string> tokens;

    std::string token;

    while (ss >> token)
    {
        tokens.push_back(token);
    }

    return tokens;
}

void UCI::commandUCINewGame()
{
    TranspositionTable::clear();

    KillerMoves::clear();

    HistoryHeuristic::clear();

    board.setStartPosition();
}

void UCI::commandDebug(
    const std::vector<std::string>& tokens)
{
    if (tokens.size() < 2)
    {
        return;
    }

    debugMode =
        (tokens[1] == "on");
}

bool UCI::isDebugMode()
{
    return debugMode;
}

void UCI::commandRegister(
    const std::vector<std::string>&)
{
    std::cout << "info string Missing Pawn does not require registration." << std::endl;
}

void UCI::commandPonderHit()
{
    TimeManager::setPonder(false);
    Search::setPonder(false);

    std::cout << "info string PonderHit received." << std::endl;
}

void UCI::commandInfo()
{
}
