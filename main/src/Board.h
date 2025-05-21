//
// Board.h
// Developed by the GAME2 Team.
//

#ifndef BOARD_H
#define BOARD_H

#include <stdbool.h>
#include <math.h>
#include <stdint.h>

#define BOARD_SIZE 10
#define ANY '|'
#define EMPTY '-'
#define WHITE 'O'
#define BLACK 'X'
#define WHITE2 'A'
#define BLACK2 'B'

typedef enum Direction {HORIZONTAL, VERTICAL, DIAGONAL_FORWARD, DIAGONAL_BACK} Direction;


typedef struct Board {
    uint16_t white[BOARD_SIZE];
    uint16_t black[BOARD_SIZE];
} Board;

void count_bit_LUT_init();

char get(const Board *board, int x, int y);

bool is_black(const Board* board, int x, int y);

bool is_white(const Board* board, int x, int y);

bool is_empty(const uint16_t* board);

bool is_full(const uint16_t* board);

void init_eval(const Board* board);

void shift_and(const Board* board, uint16_t* bb_out, char player, Direction dir, int n);

void shift_or(const Board* board, uint16_t* bb_out, char player, Direction dir, int n);

int count_sequence(const Board* board, const char* sequence, Direction dir);

void count_direction(const Board *board, Direction dir, int *sequences_white, int *sequences_black);

int count1s(const uint16_t* bit_board);

void print_board(const Board* board);

Board* create_board();

bool place_piece(Board* board, int x, int y, char player);

char check_winner(const Board* board);

bool is_board_empty(const Board* board);

bool is_board_full(const Board* board);

int has_neighbor(const Board* board, int x, int y);

Board* create_next_board(const Board* board, int move, char player);

Board* copy_board(const Board* board);


#endif //BOARD_H
