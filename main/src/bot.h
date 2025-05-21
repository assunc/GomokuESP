//
// Bot.h
// Developed by the GAME2 Team.
//

#ifndef BOT_H
#define BOT_H

#include <stdbool.h>
#include <limits.h>
#include "board.h"

#define N_OF_CHECKS 10
#define FUTILITY_MARGIN 100
#define R 3
// #define DELTA 1000

void init_bot(int t_t_cap);

int minimax(const Board* board, char player, int alpha, int beta, int depth, int* move, bool null);

int quiescence_search(const Board* board, char player, int alpha, int beta, int depth);

int bot_place_piece(const Board* board, char player, int max_depth);

int evaluate_board(const Board* board);

bool is_next_position(const Board* board, int x, int y);

int count_next_moves(const Board* board);

void find_next_moves(int* next_moves, int n_of_moves, const Board* board, char player, int threshold, int best_move);

int evaluate_move(const Board* board, int x, int y, char player);

void set_do_quiescence(bool new);

void set_better_move_order(bool new);

void reset_bot();

void free_bot();

#endif //BOT_H
