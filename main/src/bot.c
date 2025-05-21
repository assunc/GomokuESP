//
// Bot.c
// Developed by the GAME2 Team.
//
#include "bot.h"

#include <stdio.h>

#include "Board.h"
#include "hashmap.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

Map transposition_table;
bool do_quiescence = true;
bool better_move_order = true;

// initialize bot (transposition table and look up table)
void init_bot(const int t_t_cap) {
    transposition_table = init_map(t_t_cap);
    count_bit_LUT_init();
}

// activate quiescence search
void set_do_quiescence(const bool new) {
    do_quiescence = new;
}

// activate better move ordering
void set_better_move_order(const bool new) {
    better_move_order = new;
}

int total_evaluations = 0; // total number of evaluations in a game

int collisions = 0; // number of transposition table collisions
int lookups = 0; // number of successful transposition table lookups
int evaluations = 0; // number of evaluations in a search
int q_evaluations = 0; // number of quiescence evaluations in a search
int f_pruning = 0; // number of foward prunes (NOT USED)
int f_pruning1 = 0; // number of extended foward prunes (NOT USED)
int null_pruning = 0; // number of null prunes
int delta_pruning = 0; // number of delta prunes (NOT USED)
int turn_count = 0; // number of turns

// Searches best next move from a Board state
int bot_place_piece(const Board* board, const char player, const int max_depth) {
    int move = -1;
    collisions = 0;
    lookups = 0;
    evaluations = 0;
    q_evaluations = 0;
    f_pruning = 0;
    f_pruning1 = 0;
    null_pruning = 0;
    delta_pruning = 0;
    const clock_t start_time = clock();
    // move = iterative_deepening_search(board, player, max_depth);
    const int score = minimax(board, player, INT_MIN, INT_MAX, max_depth, &move, false);
    total_evaluations += evaluations;
    printf("Turn: %d\n", ++turn_count);
    printf("time: %.3f\n", (double)(clock() - start_time) / CLOCKS_PER_SEC);
    printf("score: %d\n", score);
    printf("t_table size: %d\n", transposition_table.size);
    printf("lookup:      %d\n", lookups);
    // printf("collision:   %d\n", collisions);
    printf("evaluations: %d\n", evaluations);
    printf("quiescent evaluations: %d\n", q_evaluations);
    // printf("delta pruning: %d\n", delta_pruning);
    // printf("foward pruning: %d\n", f_pruning);
    // printf("no foward pruning: %d\n", f_pruning1);
    printf("null pruning: %d\n", null_pruning);
    // empty_map(&transposition_table);
    return move;
}

// Alpha beta search a Board state
int minimax(const Board* board, const char player, int alpha, int beta, const int depth, int* move, const bool null) {
    // query transposition table
    const data* t_t_entry = get_map(&transposition_table, board); 
    if (t_t_entry != NULL && t_t_entry->n_of_pieces == count1s(board->black) + count1s(board->white)) { // avoid collisions
        if (t_t_entry->depth >= depth) { // t_table score is at least as good as required depth
            lookups++;
            return t_t_entry->score;
        }
        // t_table score is at a lower depth -> lower/upper bound estimate
        if (player == WHITE)
            alpha = alpha > t_t_entry->score ? alpha : t_t_entry->score;
        else if (player == BLACK)
            beta = beta < t_t_entry->score ? beta : t_t_entry->score;
        if (alpha > beta)
            return t_t_entry->score;
    }
    // check if position has winner
    if (check_winner(board) != '\0') return (depth+1)*evaluate_board(board);
    // horizon nodes: perform quiescence search or evaluation
    if (depth <= 0) {
        const int score = do_quiescence && !null ? quiescence_search(board, player, alpha, beta, 10) : evaluate_board(board);
        if (!null) put_map(&transposition_table, board, score, 0, -1, false); //, EXACT);
        return score;
    }
    // search
    int* next_moves = NULL;
    int best_eval;
    Board* next_board;
    int n_of_moves;
    if (player == WHITE) {
        best_eval = INT_MIN;
        if (!null && depth >= 2) { // null search
            const int null_eval = minimax(board, BLACK, alpha, beta, depth-R, NULL, true);
            alpha = alpha > null_eval ? alpha : null_eval; // max(alpha, null_eval)
            if (alpha >= beta) {
                null_pruning++;
                return null_eval; // null move pruning
            }
        }
        // search next moves
        n_of_moves = count_next_moves(board);
        next_moves = malloc(sizeof(int) * n_of_moves);
        find_next_moves(next_moves, n_of_moves, board, player, 0, t_t_entry != NULL ? t_t_entry->best_move : -1);
        for (int i = 0; i < n_of_moves && next_moves[i] >= 0; i++) {
            next_board = create_next_board(board, next_moves[i], WHITE);
            const int eval = minimax(next_board, BLACK, alpha, beta, depth - 1, NULL, null);
            free(next_board);
            if (eval > best_eval) {
                best_eval = eval;
                if (move != NULL) *move = next_moves[i];
            }
            alpha = alpha > eval ? alpha : eval; // max(alpha, eval)
            if (beta <= alpha) break; // alpha beta cutoff
        }
    } else { // player == BLACK
        best_eval = INT_MAX;
        if (!null && depth >= 2) { // null search
            const int null_eval = minimax(board, WHITE, alpha, beta, depth-R, NULL, true);
            beta = beta < null_eval ? beta : null_eval; // min(beta, null_eval)
            if (alpha >= beta) {
                null_pruning++;
                return null_eval; // null move pruning
            }
        }
        // search next moves
        n_of_moves = count_next_moves(board);
        next_moves = malloc(sizeof(int) * n_of_moves);
        find_next_moves(next_moves, n_of_moves, board, player, 0, t_t_entry != NULL ? t_t_entry->best_move : -1);
        for (int i = 0; i < n_of_moves && next_moves[i] >= 0; i++) {
            next_board = create_next_board(board, next_moves[i], BLACK);
            const int eval = minimax(next_board, WHITE, alpha, beta, depth - 1, NULL, null);
            free(next_board);
            if (eval < best_eval) {
                best_eval = eval;
                if (move != NULL) *move = next_moves[i];
            }
            beta = beta < eval ? beta : eval; // min(beta, eval)
            if (beta <= alpha) break; // alpha beta cutoff
        }
    }
    if (!null) put_map(&transposition_table, board, best_eval, depth, move == NULL ? -1 : *move, false); //, EXACT);
    free(next_moves);
    return best_eval;
}

// perform a quiescence search in a Board state
int quiescence_search(const Board* board, const char player, int alpha, int beta, const int depth) {
    // query transposition table
    const data* t_t_entry = get_map(&transposition_table, board);
    if (t_t_entry != NULL) {
        lookups++;
        return t_t_entry->score;
    }
    int best_eval = evaluate_board(board);
    if (depth < 10) q_evaluations++; 
    if (depth <= 0) return best_eval; // max depth

    Board* next_board;
    int best_move = -1;
    if (player == WHITE) {
        if (best_eval >= beta) {
            return best_eval;
        }
        // if (best_eval + DELTA < alpha) {
        //     delta_pruning++;
        //     return alpha;
        // }
        alpha = best_eval > alpha ? best_eval : alpha; // max(alpha, best_eval)

        // search next moves that score >= 1000
        const int n_of_moves = count_next_moves(board);
        int* next_moves = malloc(sizeof(int) * n_of_moves);
        find_next_moves(next_moves, n_of_moves, board, player, 1000, -1);
        if (next_moves[0] == -1) {
            free(next_moves);
            return best_eval;
        }
        for (int i = 0; i < n_of_moves && next_moves[i] >= 0; i++) {
            next_board = create_next_board(board, next_moves[i], WHITE);
            const int eval = quiescence_search(next_board, BLACK, alpha, beta, depth - 1);
            free(next_board);

            if(eval > best_eval) {
                best_eval = eval;
                best_move = next_moves[i];
            }
            if(best_eval >= beta) break;
            if(eval > alpha) alpha = eval;
        }
        free(next_moves);
    } else { // player == BLACK
        if (best_eval <= alpha) {
            return best_eval;
        }
        // if (best_eval - DELTA > beta) {
        //     delta_pruning++;
        //     return beta;
        // }
        beta = best_eval < beta ? best_eval : beta; // min(beta, best_eval)

        // search next moves that score >= 1000
        const int n_of_moves = count_next_moves(board);
        int* next_moves = malloc(sizeof(int) * n_of_moves);
        find_next_moves(next_moves, n_of_moves, board, player, 1000, -1);
        if (next_moves[0] == -1) {
            free(next_moves);
            return best_eval;
        }
        for (int i = 0; i < n_of_moves && next_moves[i] >= 0; i++) {
            next_board = create_next_board(board, next_moves[i], BLACK);
            const int eval = quiescence_search(next_board, WHITE, alpha, beta, depth - 1);
            free(next_board);

            if(eval < best_eval) {
                best_eval = eval;
                best_move = next_moves[i];
            }
            if(best_eval <= alpha) break;
            if(eval < beta) beta = eval;
        }
        free(next_moves);
    }
    put_map(&transposition_table, board, best_eval, 0, best_move, true); //, EXACT);
    return best_eval;
}

// gives a score to a board state (+ for white, - for black)
int evaluate_board(const Board* board) {
    evaluations++;
    init_eval(board);
    // check if game is over
    const char winner = check_winner(board);
    if (winner == WHITE) return 10000000;
    if (winner == BLACK) return -10000000;

    const int open_sequences_values[N_OF_CHECKS] = {
        1,      // [0]: 2 in a row open once OO---
        100,    // [1]: 3 in a row open once OOO--
        10000,  // [2]: 4 in a row open once OOOO-
        10,     // [3]: 2 in a row open twice -OO--
        8000,   // [4]: 3 in a row open twice -OOO-
        100000, // [5]: 4 in a row open twice -OOOO-
        10000,  // [6]: OO-OO sequences
        10000,  // [7]: OOO-O sequences
        1000,   // [8]: -OO-O or OO-O- sequences
        8000,   // [9]: -OO-O- sequences
    };
    init_eval(board);
    int score = 0;
    int open_sequences_white[N_OF_CHECKS];
    int open_sequences_black[N_OF_CHECKS];

    for (int i = 0; i < N_OF_CHECKS; i++) {
        open_sequences_white[i] = 0;
        open_sequences_black[i] = 0;
    }

    for (int dir = 0; dir < 4; dir++)
        count_direction(board, dir, open_sequences_white, open_sequences_black);

    for (int i = 0; i < N_OF_CHECKS; i++) {
        score += open_sequences_values[i] * (open_sequences_white[i] - open_sequences_black[i]);
    }
    return score;
}

// check if a board postition is a valid next move (has a neighboring piece)
bool is_next_position(const Board* board, const int x, const int y) {
    return !(is_black(board, x, y) || is_white(board, x, y)) && has_neighbor(board, x, y);
}

// count the number of possible next moves
int count_next_moves(const Board* board) {
    if (is_board_empty(board)) return 1;
    int next_moves = 0;
    for (int i = 0; i < BOARD_SIZE; i++)
        for (int j = 0; j < BOARD_SIZE; j++)
            if (is_next_position(board, j, i))
                next_moves++;
    return next_moves;
}

// find all possible next moves, sorts by move score, filters out bad moves
void find_next_moves(int* next_moves, const int n_of_moves, const Board* board, const char player, const int threshold, const int best_move) {
    if (next_moves == NULL) return;
    int scores[BOARD_SIZE*BOARD_SIZE]; // score for each board position (only fill possible moves)
    if (is_board_empty(board)) { // if board is empty play randomly
        next_moves[0] = rand() % (BOARD_SIZE*BOARD_SIZE);
        return;
    }
    for (int i = 0; i < n_of_moves; i++) {
        next_moves[i] = -1;
    }
    int count = 0;
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (is_next_position(board, j, i)) {
                const int score = evaluate_move(board, j, i, player);
                if (score >= threshold) {
                    scores[i * BOARD_SIZE + j] = score;
                    int n;
                    for (n = count - 1; n >= 0 && scores[next_moves[n]] < scores[i * BOARD_SIZE + j]; n--)
                        next_moves[n + 1] = next_moves[n];
                    next_moves[n + 1] = i * BOARD_SIZE + j;
                    count++;
                }
            }
        }
    }
    // removes moves 90 times worse then best move
    const int threshold_score = scores[next_moves[0]]/90;
    for (int i = 0; i < n_of_moves; i++) {
        if (next_moves[i] < -1 || next_moves[i] > BOARD_SIZE*BOARD_SIZE || scores[next_moves[i]] < threshold_score) {
            next_moves[i] = -1;
            count = i;
        }
    }
    if (best_move != -1) { // put best move from transposition table first
        for (int i = 0; i < count; i++) {
            if (next_moves[i] == best_move) {
                for (int j = i-1; j >= 0; j--) {
                    next_moves[j+1] = next_moves[j];
                }
                next_moves[0] = best_move;
                break;
            }
        }
    }
}

// gives a score to a potential move
int evaluate_move(const Board* board, const int x, const int y, const char player) {
    int score = 0;
    const char enemy = player == WHITE ? BLACK : WHITE;
    int sequences[8] = {0, 0, 0, 0, 0, 0, 0, 0}; // NW, N, NE, W, E, SW, S, SE
    int wall[8] = {0, 0, 0, 0, 0, 0, 0, 0}; // NW, N, NE, W, E, SW, S, SE
    for (int i = 1; i < 5; i++) { // NW
        if (y-i >= 0 && x-i >= 0) {
            if (sequences[0] == i-1 && get(board, x-i, y-i) == player)
                sequences[0]++;
            else if (sequences[0] == 1-i && get(board, x-i, y-i) == enemy)
                sequences[0]--;
            else if (sequences[0] == i-1 && get(board, x-i, y-i) == enemy)
                sequences[0]+=10;
            else if (sequences[0] == 1-i && get(board, x-i, y-i) == player)
                sequences[0]-=10;
        } else {
            wall[0] = i;
            break;
        }
    }
    for (int i = 1; i < 5; i++) { // N
        if (y-i >= 0) {
            if (sequences[1] == i-1 && get(board, x, y-i) == player)
                sequences[1]++;
            else if (sequences[1] == 1-i && get(board, x, y-i) == enemy)
                sequences[1]--;
            else if (sequences[1] == i-1 && get(board, x, y-i) == enemy)
                sequences[1]+=10;
            else if (sequences[1] == 1-i && get(board, x, y-i) == player)
                sequences[1]-=10;
        } else {
            wall[1] = i;
            break;
        }
    }
    for (int i = 1; i < 5; i++) { // NE
        if (y-i >= 0 && x+i < BOARD_SIZE) {
            if (sequences[2] == i-1 && get(board, x+i, y-i) == player)
                sequences[2]++;
            else if (sequences[2] == 1-i && get(board, x+i, y-i) == enemy)
                sequences[2]--;
            else if (sequences[2] == i-1 && get(board, x+i, y-i) == enemy)
                sequences[2]+=10;
            else if (sequences[2] == 1-i && get(board, x+i, y-i) == player)
                sequences[2]-=10;
        } else {
            wall[2] = i;
            break;
        }
    }
    for (int i = 1; i < 5; i++) { // W
        if (x-i >= 0) {
            if (sequences[3] == i-1 && get(board, x-i, y) == player)
                sequences[3]++;
            else if (sequences[3] == 1-i && get(board, x-i, y) == enemy)
                sequences[3]--;
            else if (sequences[3] == i-1 && get(board, x-i, y) == enemy)
                sequences[3] += 10;
            else if (sequences[3] == 1-i && get(board, x-i, y) == player)
                sequences[3] -= 10;
        } else {
            wall[3] = i;
            break;
        }
    }
    for (int i = 1; i < 5; i++) { // E
        if (x+i < BOARD_SIZE) {
            if (sequences[4] == i-1 && get(board, x+i, y) == player)
                sequences[4]++;
            else if (sequences[4] == 1-i && get(board, x+i, y) == enemy)
                sequences[4]--;
            else if (sequences[4] == i-1 && get(board, x+i, y) == enemy)
                sequences[4] += 10;
            else if (sequences[4] == 1-i && get(board, x+i, y) == player)
                sequences[4] -= 10;
        } else {
            wall[4] = i;
            break;
        }
    }
    for (int i = 1; i < 5; i++) { // SW
        if (y+i < BOARD_SIZE && x-i >= 0) {
            if (sequences[5] == i-1 && get(board, x-i, y+i) == player)
                sequences[5]++;
            else if (sequences[5] == 1-i && get(board, x-i, y+i) == enemy)
                sequences[5]--;
            else if (sequences[5] == i-1 && get(board, x-i, y+i) == enemy)
                sequences[5]+=10;
            else if (sequences[5] == 1-i && get(board, x-i, y+i) == player)
                sequences[5]-=10;
        } else {
            wall[5] = i;
            break;
        }
    }
    for (int i = 1; i < 5; i++) { // S
        if (y+i < BOARD_SIZE) {
            if (sequences[6] == i-1 && get(board, x, y+i) == player)
                sequences[6]++;
            else if (sequences[6] == 1-i && get(board, x, y+i) == enemy)
                sequences[6]--;
            else if (sequences[6] == i-1 && get(board, x, y+i) == enemy)
                sequences[6]+=10;
            else if (sequences[6] == 1-i && get(board, x, y+i) == player)
                sequences[6]-=10;
        } else {
            wall[6] = i;
            break;
        }
    }
    for (int i = 1; i < 5; i++) { // SE
        if (y+i < BOARD_SIZE && x+i < BOARD_SIZE) {
            if (sequences[7] == i-1 && get(board, x+i, y+i) == player)
                sequences[7]++;
            else if (sequences[7] == 1-i && get(board, x+i, y+i) == enemy)
                sequences[7]--;
            else if (sequences[7] == i-1 && get(board, x+i, y+i) == enemy)
                sequences[7]+=10;
            else if (sequences[7] == 1-i && get(board, x+i, y+i) == player)
                sequences[7]-=10;
        } else {
            wall[7] = i;
            break;
        }
    }
    if (better_move_order) {
        for (int i = 0; i < 8; i++) { // remove open-once sequences that wouldn't get to 5
            if (wall[i] == 1 && (abs(sequences[7-i]) == 13 || abs(sequences[7-i]) == 12)) {
                sequences[7-i] = 0;
            } else if (wall[i] == 2 && abs(sequences[7-i]) == 12) {
                sequences[7-i] = 0;
            } else if (abs(sequences[i] + sequences[7-i]) > 20 && abs(sequences[i] + sequences[7-i]) < 24) {
                sequences[i] = 0;
                sequences[7-i] = 0;
            }
        }
    }      
    for (int i = 0; i < 8; i++) {
        int temp = 0;
        switch (sequences[i]) { // _ = move place, O = ally piece, X = enemy piece, - = empty place, | = anything that blocks (wall or X if O sequence or O if X sequence)
            case 4:     // _OOOO
                temp = 1000000;
                break;
            case -4:    // _XXXX
                temp = 100000;
                break;
            case -3:    // _XXX-
                temp = 10000;
                break;
            case 3:     // _OOO-
                temp = 1000;
                break;
            case 13:    // _OOO|
            case -13:   // _XXX|
                temp = 100;
                break;
            case 2:     // _OO-
            case -2:    // _XX-
            case 12:    // _OO|
                temp = 10;
                break;
            case -12:   // _XX|
            case 1:     // _O-
                temp = 1;
                break;
            default:
        }
        if (better_move_order) { // boost scores of moves that complete sequences in the middle
            if (sequences[7-i] * sequences[i] > 0) temp *= 10;
            if (sequences[7-i] * sequences[i] > 0 && abs(sequences[7-i])%10 + abs(sequences[i])%10 >= 4) temp *= 100; // win move
            if (sequences[7-i] * sequences[i] > 0 && abs(sequences[7-i]) + abs(sequences[i]) == 3) temp *= 100; // guarantee/block win move (-OO-O-)
        }
        score += temp;
    }
    return score;
}

// restarts the bot for a new game
void reset_bot() {
    empty_map(&transposition_table);
}

// free allocated resources
void free_bot() {
    free_map(&transposition_table);
}
