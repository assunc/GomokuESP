//
// zobrist.c
// Developed by the GAME2 Team.
// Inspired by https://github.com/lemire/zobristhashing/blob/master/src/zobrist.c
//
#include <stdlib.h>


#include "zobrist.h"

// gives a random uint32
static uint32_t get32rand() {
    return
    ((uint32_t) rand() <<  0 & 0x0000FFFFull) |
    ((uint32_t) rand() << 16 & 0xFFFF0000ull);
}

// initialize the zobrist table with random numbers
void init_zobrist(zobrist_t * k) {
    for (int i = 0; i < MAX_ZOBRIST_LENGTH; i++) {
        for (int j = 0; j < 1 << BOARD_SIZE; j++) {
            k->hashtab[i][j] = get32rand();
        }
    }
}

// calculates the zobrist hash of a board state
uint32_t zobrist(const Board* board, const zobrist_t *k) {
    uint32_t h = 0;
    for (size_t i = 0; i < BOARD_SIZE; i++) {
        h ^= k->hashtab[ 2*i    % MAX_ZOBRIST_LENGTH][board->black[i] >> (16-BOARD_SIZE)];
        h ^= k->hashtab[(2*i+1) % MAX_ZOBRIST_LENGTH][board->white[i] >> (16-BOARD_SIZE)];
    }
    return h;
}