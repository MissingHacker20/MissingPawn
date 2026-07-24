#pragma once

enum class UCICommand
{
    Unknown,

    // Handshake
    UCI,
    IsReady,
    Quit,
    Stop,
    UCINewGame,

    // Position
    Position,

    // Search
    Go,

    // Options
    SetOption,

    // Debug
    Debug,

    // Registration
    Register
};