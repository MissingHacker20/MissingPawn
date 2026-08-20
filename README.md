# MissingPawn

> A modern UCI chess engine focused on the strength of pawn structure.

## About

MissingPawn is an open-source chess engine implementing the **Universal Chess Interface (UCI)** protocol. It is designed to provide strong play while serving as a platform for experimenting with modern search algorithms and evaluation techniques.

The project is under active development and new features are added regularly.

## Installation

Download the latest release and add `MissingPawn.exe` to your favorite UCI-compatible GUI.

Examples include:

- Arena
- Cute Chess
- Banksia GUI
- Fritz
- ChessBase
  
A chess engine written in C++.
## Requirements:
- C++20 compiler
- CMake

## Usage

Run the engine from any UCI-compatible chess GUI.

Or from the command line:

```bash
MissingPawn.exe
```

The engine automatically switches to UCI mode after receiving the `uci` command.

## Strength

MissingPawn includes built-in opening book knowledge and iterative-deepening search with time management. Its strength is estimated around 1600 Elo.

## License

This project is released under the MIT License.

## Author

MissingPlayer (Kacper Wieczorek)

# MissingPawn

*"Even a pawn can decide the game."*