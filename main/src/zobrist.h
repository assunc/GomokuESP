//
// zobrist.h
// Developed by the GAME2 Team.
// Inspired by https://github.com/lemire/zobristhashing/blob/master/src/zobrist.c
//

#ifndef ZOBRIST_H
#define ZOBRIST_H

#include <limits.h>
#include <stdint.h>
#include "Board.h"

enum {
    MAX_ZOBRIST_LENGTH=BOARD_SIZE*2
};

typedef struct zobrist_s {
    uint32_t hashtab[MAX_ZOBRIST_LENGTH][1 << BOARD_SIZE] ;
} zobrist_t;

void init_zobrist(zobrist_t * k);

uint32_t zobrist(const Board *board, const zobrist_t * k);

#endif //ZOBRIST_H
