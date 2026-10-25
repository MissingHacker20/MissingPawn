# Third-Party Notices

## Move generator

1. MissingPawn incorporates code derived from the Stockfish chess engine,
specifically portions of its move-generation implementation.

**Original project:**
https://github.com/official-stockfish/Stockfish

**Copyright:**
Copyright (C) 2004-2026 The Stockfish developers

**License:**
GNU General Public License version 3 or later (GPL-3.0-or-later).

2. Modifications

The Stockfish-derived implementation has been modified and adapted to
integrate with MissingPawn's internal move representation and engine
architecture. The following files contain related code or adaptations:

- `Source Code/Move/MoveGenerator.cpp`
- `Source Code/Move/MoveValidator.cpp`
- `Source Code/Move/AttackTables.cpp`
- `Source Code/Game/Fen.cpp`

Applicable Stockfish copyright notices and license terms are retained in
the relevant source files.

## Opening Book

1. MissingPawn includes the `DCbook_large.bin` Polyglot opening-book data
file, distributed as part of the Polyglot Books release. The Polyglot hash
random array is derived from python-chess's Polyglot implementation.

**Opening-book source:**
https://github.com/ChrisWhittington/polyglot-books/releases
**Polyglot random-array source:**
https://github.com/niklasf/python-chess/blob/master/chess/polyglot.py

2. Integration

The Polyglot opening-book data is included in the following file:

- `Source Code/OpeningBooks/DCbook_large.bin`
- `Source Code/OpeningBooks/PolyglotRandom.inc`