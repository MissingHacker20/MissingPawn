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