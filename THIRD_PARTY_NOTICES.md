# Third-Party Notices

## Stockfish

MissingPawn incorporates code derived from the Stockfish chess engine,
specifically portions of its move-generation implementation.

**Original project:**  
https://github.com/official-stockfish/Stockfish

**Copyright:**  
Copyright (C) 2004-2026 The Stockfish developers

**License:**  
GNU General Public License version 3 or later (GPL-3.0-or-later).

### Modifications

The Stockfish-derived implementation has been modified and adapted to
integrate with MissingPawn's internal move representation and engine
architecture. The following files contain related code or adaptations:

- `Source Code/Move/MoveGenerator.cpp`
- `Source Code/Move/MoveValidator.cpp`
- `Source Code/Move/AttackTables.cpp`
- `Source Code/Game/Fen.cpp`

Applicable Stockfish copyright notices and license terms are retained in
the relevant source files.