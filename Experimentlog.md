# MissingPawn — Experimental Features

This document describes selected experimental concepts and mechanisms developed for MissingPawn. These experiments explore alternative approaches to chess evaluation, numerical precision, and computational efficiency.

Only selected experiments considered valuable to the development of MissingPawn are documented here. Their inclusion does not imply that every aspect of a feature is complete or permanently finalized.

---

## 1. Pawn Evaluation

**Category:** Position Evaluation

### Overview

Pawn Evaluation is the central concept behind MissingPawn and one of the main reasons for the engine's creation.

The primary idea is to evaluate pawns as individual units while accounting for their relationships with neighboring friendly and enemy pawns. Rather than treating pawns solely as material, this approach aims to evaluate their characteristics and their local context on the chessboard.

### Design and Approach

The underlying assumption is that a pawn's value on the board depends on more than its material worth. Its position relative to other pawns can influence its strategic significance.

Pawn Evaluation is designed to account for pawn-related characteristics that can be identified and evaluated through the engine's implementation. The `Pawn.cpp` and `Pawn.h` files contain the mechanisms responsible for this individual-pawn evaluation.

The intended approach is to examine each pawn in relation to its local environment wherever the implemented evaluation rules make this possible.

### Role in MissingPawn

Pawn Evaluation represents the foundational direction of MissingPawn's development: exploring how pawn-specific characteristics can contribute to a more detailed positional evaluation.

It also provides a basis for further experimentation with pawn analysis, including the separate evaluation of pawn structures.

---

## 2. Pawn Structure

**Category:** Positional Evaluation

### Overview

Pawn Structure explores pawn evaluation from a different perspective. Instead of focusing primarily on individual pawns, it considers pawns as a collective arrangement that influences the position of both sides.

The original design goal was to evaluate how a pawn structure supports friendly pieces and obstructs the opponent.

### Current Approach

The current implementation evaluates how pawns control space on the chessboard.

Space control provides a way to examine the positional influence of pawns as an arrangement rather than evaluating each pawn exclusively as an individual unit. This approach focuses on the territory influenced by pawns and how their placement affects the position.

### Relationship to Pawn Evaluation

Pawn Structure complements Pawn Evaluation by addressing a different level of analysis:

- **Pawn Evaluation** examines individual pawns in relation to neighboring friendly and enemy pawns.
- **Pawn Structure** examines how pawns collectively influence space.

These approaches have different objectives and should not be considered interchangeable. Together, they represent two directions in MissingPawn's exploration of pawn-based positional evaluation.

---

## 3. Millipawns

**Category:** Numerical Precision

### Overview

Millipawns introduce a finer numerical resolution for evaluating chess positions.

In conventional centipawn-based evaluation, one pawn is worth 100 centipawns. MissingPawn uses millipawns for internal calculations, where one millipawn represents one-thousandth of a pawn's value.

### Numerical Representation

The relationship between the units is:

- 1 pawn = 1,000 millipawns
- 1 pawn = 100 centipawns
- 10 millipawns = 1 centipawn

This representation allows internal evaluation calculations to retain finer numerical increments than whole-centipawn calculations.

### Use in MissingPawn

MissingPawn uses millipawns for internal calculations while normally returning its evaluation in centipawns.

This separates the precision used during internal computation from the numerical format presented by the engine. Millipawns provide a finer unit for representing evaluation values without requiring the engine to expose that unit in its usual evaluation output.

Millipawns are a numerical representation technique rather than an independent positional evaluation algorithm. They do not, by themselves, make an evaluation more strategically accurate; their benefit is the ability to represent smaller numerical differences.