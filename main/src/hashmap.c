//
// hashmap.c
// Developed by the GAME2 Team.
// Inpired by https://xnacly.me/posts/2024/c-hash-map/
//

#include <stdlib.h>
#include "hashmap.h"

#include "zobrist.h"

zobrist_t k;

// allocate and initialize hashmap
Map init_map(const int cap) {
    Map m = {0, cap, NULL};
    m.buckets = malloc(sizeof(data) * m.cap);
    empty_map(&m);
    init_zobrist(&k);
    return m;
}

// put a new search into the transposition table
void put_map(Map *m, const Board *board, const int value, const int depth, const int best_move, const bool quiescence) {//, const NodeType type) {
    const uint32_t key = zobrist(board, &k);
    const unsigned int hkey = key % m->cap;
    const unsigned int hash2 = 11 - key % 11;
    const int n_of_pieces = count1s(board->black) + count1s(board->white);
    for(int i = 0; i < m->cap; i++) {
        const unsigned int index = (hkey + i * hash2) % m->cap;
        if (m->buckets[index].key == key) { // position is already in transposition table
            if (m->buckets[index].depth < depth) { // depth is bigger -> more accurate score -> swap
                m->buckets[index].depth = depth;
                m->buckets[index].score = value;
                m->buckets[index].best_move = best_move;
            }
            return;
        }
        if(m->buckets[index].depth == -1) { // position is not in transposition table
            m->size++;
            m->buckets[index] = (data){key, depth, n_of_pieces, best_move, quiescence, value}; 
            return;
        }
        if (m->buckets[index].depth != -1 && n_of_pieces + depth - quiescence * 10 > m->buckets[index].n_of_pieces + m->buckets[index].depth + m->buckets[index].quiescence * 10) {
            m->buckets[index] = (data){key, depth, n_of_pieces, best_move, quiescence, value}; // replace useless old entry
            return;
        }
    }
}

// query transposition table for a search
data* get_map(const Map *m, const Board *board) {
    const uint32_t key = zobrist(board, &k);
    const unsigned int hkey = key % m->cap;
    const unsigned int hash2 = 11 - key % 11;
    for(int i = 0; i < m->cap; i++) {
        const unsigned int index = (hkey + i * hash2) % m->cap;
        if(m->buckets[index].key == -1) break;
        if(m->buckets[index].key == key) return &m->buckets[index];
    }
    return NULL;
}

// empty the transposition table
void empty_map(Map *m) {
    for (int i = 0; i < m->cap; i++) {
        m->buckets[i] = (data){-1, -1, -1, -1, false, 0}; 
    }
    m->size = 0;
}

// free allocated resources
void free_map(const Map *m) {
    free(m->buckets);
}