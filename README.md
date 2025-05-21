# 🔲 ESP32 Gomoku Engine

An embedded C-based Gomoku bot that runs on the ESP32, receives board states via BLE, and computes optimal moves using a Minimax-based engine. Communicates with a PIC microcontroller using nRF24 and includes support for multiple difficulty levels and optimizations like alpha-beta pruning, transposition tables, and quiescence search.

![Platform](https://img.shields.io/badge/platform-ESP32-blue)
![Language](https://img.shields.io/badge/language-C-blue)

---

## 📑 Table of Contents

- [Features](#features)
- [Getting Started](#getting-started)
- [Repository Structure](#repository-structure)
- [Gomoku Engine](#gomoku-engine)
  - [Board Representation](#the-board-representation-bitboards)
  - [Evaluation Function](#the-evaluation-function)
  - [Alpha-Beta Pruning](#alpha-beta-pruning)
  - [Other Optimizations](#other-optimizations)
  - [Difficulty Levels](#difficulty-levels)
- [BLE Communication](#ble-communication)
- [References](#references)

---

## 🚀 Features

- Minimax AI with adjustable depth
- Alpha-beta pruning for search efficiency
- Zobrist hashing & transposition tables
- Support for quiescence search and null pruning
- Three difficulty levels
- Bitboard-based evaluation system
- BLE communication with a mobile app
- Lightweight, embedded-friendly design

---

## 🛠 Getting Started

### Requirements

- ESP32 Dev Module (with BLE support)
- nRF24L01 module (for communication with PIC)
- PlatformIO / ESP-IDF toolchain
- Companion App (for BLE interface)
- C Compiler

### Build & Flash

```bash
git clone https://github.com/assunc/GomokuESP.git
cd GomokuESP
platformio run
platformio upload
```

> You can also use `idf.py build && idf.py flash` if using ESP-IDF directly.

---

## ♟️ Gomoku Engine

### The Board Representation: Bitboards

The board is represented using two 10-element arrays of `uint16_t`, where each bit represents a cell. The 10x10 board uses the 10 MSBs of each short, sacrificing 6 bits per row for simplicity and speed in evaluation.

---

### The Evaluation Function

The engine counts all sequences of pieces with the potential to become five-in-a-row. The more promising the sequence, the higher its score. Both white (player) and black (bot) sequences are considered, with scoring:

| **Sequence**                          | **Value**    |
|--------------------------------------|--------------|
| 5 in a row (OOOOO)                   | 10,000,000   |
| 4 open twice (-OOOO-)                | 100,000      |
| 4 open once (OOOO-)                 | 10,000       |
| 5 missing middle (OO-OO, OOO-O)     | 10,000       |
| 4 open twice missing middle (-OO-O-)| 8,000        |
| 3 open twice (-OOO-)                | 8,000        |
| 4 open once missing middle (-OO-O)  | 1,000        |
| 3 open once (OOO--)                 | 100          |
| 2 open twice (-OO--)                | 10           |
| 2 open once (OO---)                 | 1            |

The engine uses shift-and operations across the board in four directions (horizontal, vertical, both diagonals). A lookup table (1024 entries) helps count sequences quickly.

---

### Alpha-Beta Pruning

Minimax search is optimized using alpha-beta pruning. Two bounds, `alpha` and `beta`, are used to eliminate unnecessary branches. Complexity is reduced from `O(b^d)` to `O(b^{d/2})` with good move ordering.

---

### Other Optimizations

#### Move Ordering

Moves are only considered if adjacent to existing pieces. Moves are scored based on how well they:
- Extend own sequences
- Block enemy sequences

### Partial Move Scores per Direction

| **Sequence**                          | **Value**    |
|--------------------------------------|--------------|
| 4 ally pieces                        | 1,000,000    |
| 4 opponent pieces                    | 100,000      |
| 3 enemy pieces, open                | 10,000       |
| 3 ally pieces, open                 | 1,000        |
| 3 ally or enemy pieces, closed      | 100          |
| 2 ally or enemy pieces, open        | 10           |
| 2 ally pieces, closed               | 10           |
| 2 enemy pieces, closed              | 1            |
| 1 ally piece, open                  | 1            |

Score is increased when:
- Combine opposite-direction lines (×10)
- Create four-open-twice or five-in-a-row (×100)

Only top-scoring moves (within 1/90th of the best) are searched.

#### Transposition Table

To reduce repeated calculations, the engine uses a hash-based transposition table. Each entry contains:

| **Field**       | **Type**       | **Description**                    |
|----------------|----------------|------------------------------------|
| Key            | `uint32_t`     | 32-bit Zobrist hash                |
| Depth          | `int8_t`       | Search depth of entry              |
| Piece count    | `int8_t`       | Used to verify identity            |
| Best move      | `int8_t`       | Best move found from this position |
| From quiescence| `bool`         | Whether result was quiescence-based|
| Score          | `int`          | Evaluation result                  |

Only entries with zero likelihood of reuse are replaced.

#### Quiescence Search

If a position has a volatile threat (score ≥ 1000), a quiescence search explores further to avoid the horizon effect. Limited to depth 10 to manage cost.

#### Null Pruning

Simulates skipping a move. If opponent can't improve, branch is pruned. Does not use quiescence or store in transposition table.

---

### Difficulty Levels

Difficulty is set via the companion app over BLE:

| **Level** | **Depth** | **Quiescence** | **Move Ordering** |
|-----------|-----------|----------------|--------------------|
| Easy      | 1         | ❌             | Basic              |
| Medium    | 3         | ❌             | Full               |
| Hard      | 5         | ✅             | Full               |

---

## 📡 BLE Communication

Uses the NimBLE stack on the ESP32. The following BLE services are available:

1. **Bot Service** (read/write)  
   Send board state, receive move

2. **Search Depth Service** (read/write)  
   Get or set current bot depth

3. **Winner Service** (read-only)  
   Query game result (draw/win/loss)

4. **Safety System Service** (notify)  
   App receives safety-triggered updates or move completion signals

---

## 🔗 References

- [ChessProgrammingWiki](https://www.chessprogramming.org/)
- [Zobrist Hashing](https://github.com/ZobristHashingRepo)
- [C HashMap](https://github.com/Xnacly/c-hashmap)
- [Double Hashing - GeeksForGeeks](https://www.geeksforgeeks.org/double-hashing/)
- [ESP32 NimBLE BLE Library - Espressif](https://github.com/espressif/esp-nimble)

---

> For questions, issues, or feature requests, please open an [Issue](https://github.com/yourusername/gomoku-engine/issues).
