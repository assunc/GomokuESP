//
// Board.c
// Developed by the GAME2 Team.
//
#include "Board.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint8_t BitsSetTable[1 << BOARD_SIZE];

// Initialise the lookup table for counting set bits
void count_bit_LUT_init() {
    BitsSetTable[0] = 0;
    for (int i = 0; i < 1 << BOARD_SIZE; i++)
        BitsSetTable[i] = (i & 1) + BitsSetTable[i / 2];
}

void print_board(const Board* board) {
    if (board == NULL) return;
    printf("   ");
    for (int j = 0; j < BOARD_SIZE; j++) {
        printf(" %2d ", j);
    }
    printf("\n");
    for (int i = 0; i < BOARD_SIZE; i++) {
        // for (int j = 0; j < BOARD_SIZE; j++) {
        //     printf("+---");
        // }
        // printf("+\n");
        printf("%2d ", i);
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (is_black(board, j, i)) {
                printf("| %c ", BLACK);
            } else if (is_white(board, j, i)) {
                printf("| %c ", WHITE);
            } else {
                printf("| %c ", EMPTY);
            }
        }
        printf("|\n");
    }
    // for (int j = 0; j < BOARD_SIZE; j++) {
    //     printf("+---");
    // }
    // printf("+\n");
}

// allocate memoy and create an empty board 
Board* create_board() {
    Board* board = malloc(sizeof(Board));
    for (int i = 0; i < BOARD_SIZE; i++) {
        board->black[i] = 0;
        board->white[i] = 0;
    }
    return board;
}

// get the piece from a specified position
char get(const Board *board, const int x, const int y) {
    if (is_black(board, x, y)) return BLACK;
    if (is_white(board, x, y)) return WHITE;
    return EMPTY;
}

// check if a position has a black piece
bool is_black(const Board* board, const int x, const int y) {
    return board->black[y] & 1 << (15-x);
}

// check if a position has a white piece
bool is_white(const Board* board, const int x, const int y) {
    return board->white[y] & 1 << (15-x);
}

// check if a bit board is empty (all 0)
bool is_empty(const uint16_t* board) {
    for (int i = 0; i < BOARD_SIZE; i++)
        if (board[i])
            return false;
    return true;
}

// check if a bit board is full (all 1)
bool is_full(const uint16_t* board) {
    for (int i = 0; i < BOARD_SIZE; i++)
        if ((uint16_t)~board[i] >> (16-BOARD_SIZE))
            return false;
    return true;
}

// global bitboards to avoid repeat calculations
uint16_t bb_empty[BOARD_SIZE]; // empty bit board (1 where no piece)
uint16_t bb_any[BOARD_SIZE]; // any bit board (1 where any piece) (NOT USED)

uint16_t bb_white2[BOARD_SIZE]; // bit board for sequence of 2 white pieces (recalculated for every direction)
uint16_t bb_black2[BOARD_SIZE]; // bit board for sequence of 2 black pieces (recalculated for every direction)

// initialize bitboards necessary fr a board evaluation
void init_eval(const Board* board) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        bb_empty[i] = ~(board->black[i] | board->white[i] | ((1<<(16-BOARD_SIZE))-1));
        bb_any[i] = board->black[i] | board->white[i];
    }
}

// returns bit board associated with a char
const uint16_t* get_bb(const Board* board, const char player) {
    switch (player) {
        case BLACK:
            return board->black;
        case WHITE:
            return board->white;
        case BLACK2:
            return bb_black2;
        case WHITE2:
            return bb_white2;
        case EMPTY:
            return bb_empty;
        case ANY:
        default:
            return bb_any;
    }
}

// bit shifts bitboard from char player n times in direction dir then bitwise AND with bb_out
void shift_and(const Board* board, uint16_t* bb_out, const char player, const Direction dir, const int n) {
    const uint16_t* bb_in = get_bb(board, player);
    switch (dir) {
        case HORIZONTAL:
            for (int i = 0; i < BOARD_SIZE; i++) {
                bb_out[i] &= bb_in[i] << n;
            }
        break;
        case VERTICAL:
        for (int i = 0; i < BOARD_SIZE; i++) {
            if (i < n) bb_out[i] = 0;
            else bb_out[i] &= bb_in[i-n];
        }
        break;
        case DIAGONAL_FORWARD:
        for (int i = 0; i < BOARD_SIZE; i++) {
            if (i < n) bb_out[i] = 0;
            else bb_out[i] &= bb_in[i-n] >> n;
        }
        break;
        case DIAGONAL_BACK:
        for (int i = 0; i < BOARD_SIZE; i++) {
            if (i < n) bb_out[i] = 0;
            else bb_out[i] &= bb_in[i-n] << n;
        }
        break;
    }
}

// bit shifts bitboard from char player n times in direction dir then bitwise OR with bb_out
// NEVER USED
void shift_or(const Board* board, uint16_t* bb_out, const char player, const Direction dir, const int n) {
    uint16_t bb_in[BOARD_SIZE];
    switch (player) {
        case BLACK:
            for (int i = 0; i < BOARD_SIZE; i++)
                bb_in[i] = board->black[i];
        break;
        case WHITE:
            for (int i = 0; i < BOARD_SIZE; i++)
                bb_in[i] = board->white[i];
        break;
        case EMPTY:
            for (int i = 0; i < BOARD_SIZE; i++)
                bb_in[i] = ~(board->black[i] | board->white[i]);
        break;
        case ANY:
            for (int i = 0; i < BOARD_SIZE; i++)
                bb_in[i] = board->black[i] | board->white[i];
        break;
        default:
            return;
    }
    switch (dir) {
        case HORIZONTAL:
            for (int i = 0; i < BOARD_SIZE; i++) {
                bb_out[i] |= bb_in[i] << n;
            }
        break;
        case VERTICAL:
            for (int i = 0; i < BOARD_SIZE; i++) {
                if (i < n) bb_out[i] = 0;
                else bb_out[i] |= bb_in[i-n];
            }
        break;
        case DIAGONAL_FORWARD:
            for (int i = 0; i < BOARD_SIZE; i++) {
                if (i < n) bb_out[i] = 0;
                else bb_out[i] |= bb_in[i-n] >> n;
            }
        break;
        case DIAGONAL_BACK:
            for (int i = 0; i < BOARD_SIZE; i++) {
                if (i < n) bb_out[i] = 0;
                else bb_out[i] |= bb_in[i-n] << n;
            }
        break;
    }
}

// count set bits in a bit board
int count1s(const uint16_t *bit_board) {
    int count = 0;
    for (int i = 0; i < BOARD_SIZE; i++)
        count += BitsSetTable[bit_board[i] >> (16-BOARD_SIZE)];
    return count;
}

// performs a shift-and operation according to a sequence of characters associated with bit boards
void shift_and_sequence(const Board* board, uint16_t* res, const char* sequence, const unsigned int seq_len, const Direction dir, int *count) {
    for (int i = 0; i < seq_len; i++) {
        if (i > 0) {
            shift_and(board, res, sequence[i], dir, i);
            if (is_empty(res)) return;
        }
        if (sequence[i] == WHITE2 || sequence[i] == BLACK2) i++; // skip a char if using one of the 2 sequence bit boards
    }
    *count += count1s(res);
}

// counts the number of a sequence in the board
int count_sequence(const Board* board, const char* sequence, const Direction dir) {
    uint16_t res[BOARD_SIZE];
    memcpy(res, get_bb(board, sequence[0]), sizeof(uint16_t)*BOARD_SIZE);
       
    int count = 0;
    shift_and_sequence(board, res, sequence, strlen(sequence), dir, &count);
    return count;
}

// count all sequences in a direction
void count_direction(const Board *board, const Direction dir, int *sequences_white, int *sequences_black) {
    // pre calculate the 2 sequence bit boards for this direction
    memcpy(bb_white2, board->white, sizeof(uint16_t)*BOARD_SIZE);
    shift_and(board, bb_white2, WHITE, dir, 1);
    memcpy(bb_black2, board->black, sizeof(uint16_t)*BOARD_SIZE);
    shift_and(board, bb_black2, BLACK, dir, 1);

    if (!is_empty(bb_white2)) { // if there are sequences of 2
        sequences_white[0] += count_sequence(board, "AA---", dir) + count_sequence(board, "---AA", dir);
        sequences_white[3] += count_sequence(board, "-AA--", dir) + count_sequence(board, "--AA-", dir);
        sequences_white[6] += count_sequence(board, "AA-AA", dir);
        sequences_white[8] += count_sequence(board, "-AA-O", dir) + count_sequence(board, "-O-AA", dir) +
                                   count_sequence(board, "AA-O-", dir) + count_sequence(board, "O-AA-", dir);
        sequences_white[9] += count_sequence(board, "-AA-O-", dir) + count_sequence(board, "-O-AA-", dir);
        if (count_sequence(board, "AAO", dir) > 0) { // if there are sequences of 3
            sequences_white[1] += count_sequence(board, "AAO--", dir) + count_sequence(board, "--AAO", dir);
            sequences_white[2] += count_sequence(board, "AAAA-", dir) + count_sequence(board, "-AAAA", dir);
            sequences_white[4] += count_sequence(board, "-AAO-", dir);
            sequences_white[5] += count_sequence(board, "-AAAA-", dir);
            sequences_white[7] += count_sequence(board, "AAO-O", dir) + count_sequence(board, "O-AAO", dir);
        }
    }
    if (!is_empty(bb_black2)) { // if there are sequences of 2
        sequences_black[0] += count_sequence(board, "BB---", dir) + count_sequence(board, "---BB", dir);
        sequences_black[3] += count_sequence(board, "-BB--", dir) + count_sequence(board, "--BB-", dir);
        sequences_black[6] += count_sequence(board, "BB-BB", dir);
        sequences_black[8] += count_sequence(board, "-BB-X", dir) + count_sequence(board, "-X-BB", dir) +
                                   count_sequence(board, "BB-X-", dir) + count_sequence(board, "X-BB-", dir);
        sequences_black[9] += count_sequence(board, "-BB-X-", dir) + count_sequence(board, "-X-BB-", dir);
        if (count_sequence(board, "BBX", dir) > 0) { // if there are sequences of 3
            sequences_black[1] += count_sequence(board, "BBX--", dir) + count_sequence(board, "--BBX", dir);
            sequences_black[2] += count_sequence(board, "BBBB-", dir) + count_sequence(board, "-BBBB", dir);
            sequences_black[4] += count_sequence(board, "-BBX-", dir);
            sequences_black[5] += count_sequence(board, "-BBBB-", dir);
            sequences_black[7] += count_sequence(board, "BBX-X", dir) + count_sequence(board, "X-BBX", dir);
        }
    }
}

// place a piece in a posii=tion in a Board
bool place_piece(Board* board, const int x, const int y, const char player) {
    if (board == NULL) return false;
    if (player != WHITE && player != BLACK) return false;
    if (is_black(board, x, y) || is_white(board, x, y)) return false;
    switch (player) {
        case WHITE:
            board->white[y] |= 1 << (15-x);
            break;
        case BLACK:
            board->black[y] |= 1 << (15-x);
        default: ;
    }
    return true;
}

// checks if a Board has a winner/draw
char check_winner(const Board* board) {
    if (board == NULL) return '\0';
    for (int dir = 0; dir < 4; dir++) {
        if (count_sequence(board, "OOOOO", dir)) return WHITE;
        if (count_sequence(board, "XXXXX", dir)) return BLACK;
    }
    if (is_board_full(board)) return EMPTY;
    return '\0';
}

// checks if a board is empty (all 0)
bool is_board_empty(const Board* board) {
    for (int i = 0; i < BOARD_SIZE; i++)
        if (board->black[i] | board->white[i])
            return false;
    return true;
}

// checks if a board is full (all 1)
bool is_board_full(const Board* board) {
    for (int i = 0; i < BOARD_SIZE; i++)
        if ((uint16_t)~(board->black[i] | board->white[i]) >> (16-BOARD_SIZE))
            return false;
    return true;
}

// check if a position has a neighboring piece
int has_neighbor(const Board* board, const int x, const int y) {
    for (int i = (y-1 >= 0 ? y-1 : 0); i < (y+2 <= BOARD_SIZE ? y+2 : BOARD_SIZE); i++)
        for (int j = (x-1 >= 0 ? x-1 : 0); j < (x+2 <= BOARD_SIZE ? x+2 : BOARD_SIZE); j++)
            if (is_black(board, j, i) || is_white(board, j, i))
                return true;
    return false;
}

// create a new Board state from a Board and a move
Board* create_next_board(const Board* board, const int move, const char player) {
    Board* next_board = copy_board(board);
    place_piece(next_board, move%BOARD_SIZE, move/BOARD_SIZE, player);
    return next_board;
}

// allocates memory and copies a Board
Board* copy_board(const Board* board) {
    Board* new_board = malloc(sizeof(Board));
    memcpy(new_board, board, sizeof(Board));
    return new_board;
}



