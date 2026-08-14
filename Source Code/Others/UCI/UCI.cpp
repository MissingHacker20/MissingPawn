#include "Others/UCI/UCI.h"
#include "Others/TimeManager.h"
#include "Others/UCI/UCIOptions.h"
#include "Engine/Search.h"
#include "Engine/KillerMoves.h"
#include "Engine/HistoryHeuristic.h"
#include "Game/Fen.h"
#include "Game/GameHistory.h"
#include "Others/UCI/UCIParser.h"
#include "Others/UCI/UCIPositionParser.h"
#include "Others/UCI/UCIOptionParser.h"
#include "OpeningBooks/OpeningBooks.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>

Board UCI::board;

bool UCI::debugMode = false;

std::thread UCI::searchThread;
std::atomic<bool> UCI::searchActive{false};

// Globalna książka otwarć
static std::vector<BookEntry> g_openingBook;

bool UCI::executeCommand(
    const std::string& command)
{
    auto tokens = tokenize(command);

    if (tokens.empty())
    {
        return true;
    }

    std::string commandName = tokens[0];
    std::transform(commandName.begin(), commandName.end(), commandName.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    // Podczas trwającego searchu ignorujemy komendy, które modyfikują
    // planszę / tablice (wyścigi danych z wątkiem przeszukującym).
    // "go" jest ignorowane, by nie uruchamiać drugiego wątku searchu.
    if (searchActive &&
        (commandName == "position" ||
         commandName == "setoption" ||
         commandName == "ucinewgame" ||
         commandName == "go"))
    {
        std::cout << "info string Ignorowanie komendy podczas searchu." << std::endl;
        return true;
    }

    if (commandName == "uci")
    {
        commandUCI();
        return true;
    }

    if (commandName == "isready")
    {
        commandIsReady();
        return true;
    }

    if (commandName == "quit")
    {
        commandQuit();
        return false;
    }

    if (commandName == "position")
    {
        if (tokens.size() < 2)
        {
            std::cout << "info string Missing position arguments." << std::endl;
        }
        else
        {
            UCIPositionParser::parse(
                board,
                tokens);
            std::cout << "info string Position updated." << std::endl;
        }
        return true;
    }

    if (commandName == "go")
    {
        commandGo(tokens);
        return true;
    }

    if (commandName == "stop")
    {
        commandStop();
        return true;
    }

    if (commandName == "ucinewgame")
    {
        commandUCINewGame();
        return true;
    }

    if (commandName == "setoption")
    {
        if (tokens.size() < 2)
        {
            std::cout << "info string Missing option name." << std::endl;
        }
        else
        {
            UCIOptionParser::parse(tokens);
            std::cout << "info string Option processed." << std::endl;
        }
        return true;
    }

    if (commandName == "debug" || commandName == "debugon" || commandName == "debugoff")
    {
        std::vector<std::string> debugTokens = tokens;
        if (commandName == "debugon")
        {
            debugTokens = {"debug", "on"};
        }
        else if (commandName == "debugoff")
        {
            debugTokens = {"debug", "off"};
        }

        commandDebug(debugTokens);
        return true;
    }

    if (commandName == "ponderhit")
    {
        commandPonderHit();
        return true;
    }

    if (commandName == "register")
    {
        commandRegister(tokens);
        return true;
    }

    if (commandName == "help")
    {
        std::cout << "info string Supported UCI commands: uci, isready, position, go, stop, ucinewgame, setoption, debug, ponderhit, register, quit" << std::endl;
        return true;
    }

    std::cout << "info string Unknown command: " << tokens[0] << std::endl;
    return true;
}

void UCI::run()
{
    UCIOptions::initialize();

    // Inicjalizuj książkę otwarć
    initializeBook();
    g_openingBook = buildOpeningBook();
    
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

    // Na koniec upewniamy się, że wątek searchu został dołączony
    // (np. przy zamknięciu wejścia / EOF zamiast jawnego "quit").
    TimeManager::stop();
    if (searchThread.joinable())
    {
        searchThread.join();
    }
}

void UCI::commandUCI()
{
    std::cout << "id name MissingPawn v1" << std::endl;
    std::cout << "id author Missing Player" << std::endl;
    std::cout << "option name Hash type spin default 64 min 1 max 4096" << std::endl;
    std::cout << "option name Ponder type check default false" << std::endl;
    std::cout << "uciok" << std::endl;
}

void UCI::commandIsReady()
{
    std::cout << "readyok" << std::endl;
}

void UCI::commandQuit()
{
    TimeManager::stop();

    if (searchThread.joinable())
    {
        searchThread.join();
    }

    std::cout << "bye" << std::endl;
    std::cout.flush();
}

void UCI::commandGo(
    const std::vector<std::string>& tokens)
{
    GoParameters params =
    UCIParser::parseGo(tokens);

    //--------------------------------------------------
    // Sprawdź książkę otwarć
    //--------------------------------------------------

    if (hasBookMove(g_openingBook, board.getZobristKey()))
    {
        Move bookMove = getBookMove(g_openingBook, board);
        if (bookMove.from != Square::None)
        {
            std::cout << "info string Book move played." << std::endl;
            std::cout << "bestmove " << bookMove.toUCI() << std::endl;
            return;
        }
    }

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

//--------------------------------------------------
    // Oblicz limit czasu na ten ruch
    //--------------------------------------------------
    //
    // WAŻNE: limit czasu (z wtime/btime) ustawiamy TYLKO wtedy, gdy nie
    // podano jawnej głębokości (depth). W przeciwnym razie "go depth N"
    // zostało by przerwane po upływie czasu, zanim osiągnie żądaną głębokość
    // (stąd "kulawa" głębokość — raz 6, raz 5, raz 8...).
    // "movetime" jest twardym limitem i jest obsługiwany osobno w
    // TimeManager::shouldStop() (moveTime >= 0), więc działa zawsze.
    if (!params.infinite && params.depth == 0 && !TimeManager::hasTimeControl())
    {
        ChessColor side = board.getSideToMove();
        TimeManager::setTimeLimit(
            TimeManager::calculateTimeLimit(side));
    }

    //--------------------------------------------------
    // Automatyczna głębokość jeśli nie podano
    //--------------------------------------------------

    int depth;

    if (params.depth > 0)
    {
        depth = params.depth;
    }
    else if (params.infinite)
    {
        depth = 64;
    }
    else if (TimeManager::hasTimeControl())
    {
        // Przeszukuj jak najgłębsze - TimeManager::shouldStop() przerwie wyszukiwanie
        // gdy upłynie czas. Ustawiamy maksymalną głębokość.
        depth = 64;
        
        // Ustaw limit czasu jako twardy limit (bezpiecznik)
        ChessColor side = board.getSideToMove();
        TimeManager::setTimeLimit(
            TimeManager::calculateTimeLimit(side));
    }
    else
    {
        // Bez kontroli czasu — oblicz głębokość na podstawie czasu
        ChessColor side = board.getSideToMove();
        depth = TimeManager::calculateDepthFromTime(board, side);
    }

    //--------------------------------------------------
    // Wyszukiwanie mata (go mate N)
    //--------------------------------------------------

    if (params.mateDepth > 0)
    {
        // Aby znaleźć mata w N ruchach, musimy przeszukać co najmniej
        // 2N-1 półruchów (N ruchów strony szukającej mata).
        depth = std::max(depth, params.mateDepth * 2);
    }

    if (UCI::isDebugMode())
    {
        std::cout << "info string go depth=" << depth
                  << " movetime=" << params.moveTime
                  << " nodes=" << params.nodeLimit
                  << " mate=" << params.mateDepth
                  << " infinite=" << (params.infinite ? "true" : "false")
                  << std::endl;
    }

    //--------------------------------------------------
    // Uruchom search w osobnym wątku, by główna pętla UCI mogła
    // w każdej chwili przyjąć komendę "stop" (i ją obsłużyć).
    // findBestMove sam wypisuje "bestmove ..." na stdout.
    //--------------------------------------------------
    if (searchThread.joinable())
    {
        searchThread.join();
    }

    searchActive = true;
    searchThread = std::thread([depth]()
    {
        Search::findBestMove(UCI::board, depth);
        searchActive = false;
    });
}

void UCI::commandStop()
{
    // Natychmiastowe przerwanie searchu: ustawiamy flagę, a wątek
    // przeszukujący wykryje ją przy najbliższym sprawdzeniu i się zakończy.
    TimeManager::stop();

    if (searchThread.joinable())
    {
        searchThread.join();
    }

    std::cout << "info string Search stop requested." << std::endl;
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
    KillerMoves::clear();
    
    HistoryHeuristic::clear();
    
    board.setStartPosition();
    
    GameHistory::clear();
    
    GameHistory::pushPosition(
        board.getZobristKey());

    std::cout << "info string New game initialized." << std::endl;
}

void UCI::commandDebug(
    const std::vector<std::string>& tokens)
{
    if (tokens.size() < 2)
    {
        std::cout << "info string Debug mode is "
                  << (debugMode ? "ON" : "OFF")
                  << std::endl;
        return;
    }

    std::string mode = tokens[1];
    std::transform(mode.begin(), mode.end(), mode.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (mode == "on" || mode == "true" || mode == "1")
    {
        debugMode = true;
    }
    else if (mode == "off" || mode == "false" || mode == "0")
    {
        debugMode = false;
    }
    else
    {
        debugMode = !debugMode;
    }

    std::cout << "info string Debug mode is "
              << (debugMode ? "ON" : "OFF")
              << std::endl;
}

bool UCI::isDebugMode()
{
    return debugMode;
}

void UCI::commandRegister(
    const std::vector<std::string>&)
{
    std::cout << "info string MissingPawn does not require registration." << std::endl;
}

void UCI::commandPonderHit()
{
    TimeManager::setPonder(false);
    Search::setPonder(false);

    std::cout << "info string PonderHit received." << std::endl;
}
