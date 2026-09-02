#include "Others/UCI/UCIInfoPrinter.h"

#include "Move.h"

#include <iostream>
#include <cstdint>

void UCIInfoPrinter::depth(
    int depth)
{
    std::cout
        << "info depth "
        << depth
        << '\n';
}

void UCIInfoPrinter::scoreCP(
    int score)
{
    std::cout
        << "info score cp "
        << score / 10
        << '\n';
}

void UCIInfoPrinter::scoreMate(
    int mate)
{
    std::cout
        << "info score mate "
        << mate
        << '\n';
}

void UCIInfoPrinter::nodes(
    uint64_t nodes)
{
    std::cout
        << "info nodes "
        << nodes
        << '\n';
}

void UCIInfoPrinter::nps(
    uint64_t nps)
{
    std::cout
        << "info nps "
        << nps
        << '\n';
}

void UCIInfoPrinter::time(
    uint64_t ms)
{
    std::cout
        << "info time "
        << ms
        << '\n';
}

void UCIInfoPrinter::pv(
    const std::string& pv)
{
    std::cout
        << "info pv "
        << pv
        << '\n';
}

void UCIInfoPrinter::currentMove(
    const Move& move,
    int moveNumber)
{
    std::cout
        << "info currmove "
        << move.toUCI()
        << " currmovenumber "
        << moveNumber
        << '\n';
}

void UCIInfoPrinter::string(
    const std::string& text)
{
    std::cout
        << "info string "
        << text
        << '\n';
}

void UCIInfoPrinter::bestMove(
    const Move& move)
{
    std::cout
        << "bestmove "
        << move.toUCI()
        << '\n';
}