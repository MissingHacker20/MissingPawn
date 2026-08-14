#pragma once

#include <string>
#include <cstdint>

class Move;

class UCIInfoPrinter
{
public:

    static void depth(
        int depth);

    static void scoreCP(
        int score);

    static void scoreMate(
        int mate);

    static void nodes(
        uint64_t nodes);

    static void nps(
        uint64_t nps);

    static void time(
        uint64_t ms);

    static void pv(
        const std::string& pv);

    static void currentMove(
        const Move& move,
        int moveNumber);

    static void string(
        const std::string& text);

    static void bestMove(
        const Move& move);
};