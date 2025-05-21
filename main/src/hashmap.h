//
// hashmap.h
// Developed by the GAME2 Team.
// Inpired by https://xnacly.me/posts/2024/c-hash-map/
//

#ifndef HASHMAP_H
#define HASHMAP_H

#include <stdbool.h>

#include "Board.h"

#define NO_SCORE -1163005939
#define BASE 0x811c9dc5
#define PRIME 0x01000193

typedef struct data {
    uint32_t key;
    int8_t depth;
    int8_t n_of_pieces;
    int8_t best_move;
    bool quiescence;
    int score;
} data;

typedef struct Map { int size; int cap; data *buckets; } Map;

Map init_map(int cap);

void put_map(Map *m, const Board *board, int value, int depth, int best_move, bool quiescence);

data* get_map(const Map *m, const Board *board);

void empty_map(Map *m);

void free_map(const Map *m);

#endif //HASHMAP_H
