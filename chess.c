// Harry Ahearn
// simple p2p chess game, learning sockets in C
// usage: ./chess w to connect, ./chess b to host
// (no it doesn't have castling support)

#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

#define AC_BLACK "\x1b[30m"
#define AC_RED "\x1b[31m"
#define AC_GREEN "\x1b[32m"
#define AC_YELLOW "\x1b[33m"
#define AC_BLUE "\x1b[34m"
#define AC_MAGENTA "\x1b[35m"
#define AC_CYAN "\x1b[36m"
#define AC_WHITE "\x1b[37m"
#define AC_NORMAL "\x1b[m"

#define BUFFER sizeof(int) * 2
#define SA struct sockaddr

#define BLACK_PORT 1024
#define ADDRESS "localhost:BLACK_PORT"

typedef enum Piece {WHITE_PAWN, WHITE_KNIGHT, WHITE_BISHOP, WHITE_ROOK,
    WHITE_QUEEN, WHITE_KING, BLACK_PAWN, BLACK_KNIGHT, BLACK_BISHOP, BLACK_ROOK,
    BLACK_QUEEN, BLACK_KING, EMPTY} Piece;

Piece*  init_board();
void    print_board(Piece* board);
char    piece_to_char(Piece piece);
int     is_valid_position(char* str);
void    move_from_to(Piece* board, char* from, char* to);
void    display_directions();
int     is_white(Piece piece);
int     is_black(Piece piece);
void    separate_move(char* move, char* from, char* to);
void    err_check(int val, int err, char* msg);
void    play_as_black(Piece* board);
void    play_as_white(Piece* board);

int main(int argc, char* argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: ./chess w or ./chess b\n");
        exit(1);
    }

    Piece* board = init_board();
    print_board(board);

    char black_or_white = argv[1][0];
    if (black_or_white == 'b')
        play_as_black(board);
    else
        play_as_white(board);

    free(board);
    return 0;
}

void play_as_white(Piece* board)
{
    // establish socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    err_check(sockfd, -1, "sockfd");
    int ret; // used for setsockopt, compiler should optimize this away
    err_check(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &ret, sizeof(int)),
        -1, "setsockopt");

    // make address
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(BLACK_PORT);

    // connect to black
    err_check(inet_pton(AF_INET, ADDRESS, &addr.sin_addr), -1, "inet_pton");
    err_check(connect(sockfd, (SA *) &addr, sizeof(addr)), -1, "connect");

    int not_quitting = 1;
    char recvline[BUFFER], move[BUFFER];
    char from[2], to[2];
    while (not_quitting) {
        // get user move
        int not_valid_move = 1;
        while(not_valid_move) {
            printf("Move: "); // ask for move
            fgets(move, sizeof(move), stdin);
            fflush(stdin);

            if (strcmp(move, "quit\n") == 0) { // if quit, set flag & break loop
                not_quitting = 0;
                write(sockfd, "quit", strlen(move));
                break;
            }

            from[0] = move[0]; // get from and to
            from[1] = move[1];
            to[0] = move[2];
            to[1] = move[3];

            if (is_valid_position(from) && is_valid_position(to))
                not_valid_move = 0; // exit loop
        }

        if (!not_quitting) // quit check
            break;

        // make and send move
        move_from_to(board, from, to);
        print_board(board);
        write(sockfd, move, strlen(move));

        // get black move
        int n = read(sockfd, recvline, BUFFER);
        err_check(n, -1, "n read");
        from[0] = recvline[0]; // get from and to
        from[1] = recvline[1];
        to[0] = recvline[2];
        to[1] = recvline[3];

        // make black move
        move_from_to(board, from, to);
        print_board(board);
        printf("Black's move: %s\n", recvline);
    }

    close(sockfd);
}

void play_as_black(Piece* board)
{
    // establish socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    err_check(sockfd, 1, "sockfd");
    const int enable = 1;
    err_check(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable,
        sizeof(int)), -1, "setsockopt");

    // make address
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(BLACK_PORT);

    // bind socket to address, listen and accept
    err_check(bind(sockfd, (SA *) &addr, sizeof(addr)), -1, "bind");
    err_check(listen(sockfd, 1), -1, "listen");
    int whitefd = accept(sockfd, (SA *) NULL, NULL); // connect to white

    int not_quitting = 1;
    char recvline[BUFFER], move[BUFFER];
    char from[2], to[2];
    while (not_quitting) { // read move, respond with own move loop
        int n = read(whitefd, recvline, BUFFER);
        err_check(n, -1, "n read");
        printf("White's move: %s\n", recvline);

        // interpret move
        from[0] = recvline[0];
        from[1] = recvline[1];
        to[0] = recvline[2];
        to[1] = recvline[3];

        // make move
        move_from_to(board, from, to);
        print_board(board);

        // get user move
        int not_valid_move = 1;
        while(not_valid_move) {
            printf("Move: "); // ask for move
            fgets(move, sizeof(move), stdin);
            fflush(stdin);

            if (strcmp(move, "quit\n") == 0) { // if quit, set flag & break loop
                not_quitting = 0;
                write(whitefd, "quit", strlen(move));
                break;
            }

            from[0] = move[0]; // get from and to
            from[1] = move[1];
            to[0] = move[2];
            to[1] = move[3];

            if (is_valid_position(from) && is_valid_position(to))
                not_valid_move = 0; // exit loop
        }

        if (!not_quitting) // quit check
            break;

        // make and send move
        move_from_to(board, from, to);
        print_board(board);
        write(whitefd, move, strlen(move));
    }

    close(whitefd);
    close(sockfd);
}

// outputs board with white at bottom of screen
void print_board(Piece* board)
{
    for (int i = 0; i < 8; i++) // top
        printf("---");
    printf("\n");

    for (int i = 7; i >= 0; i--) { // board contents
        for (int j = 0; j < 8; j++) {
            Piece p = board[i * 8 + j];
            if (is_white(p))
                printf("%s|%c|", AC_WHITE, piece_to_char(p));
            else if (is_black(p))
                printf("%s|%c|", AC_MAGENTA, piece_to_char(p));
            else
                printf("%s|-|", AC_BLUE);
        }
        printf("%s\n", AC_NORMAL);
    }

    for (int i = 0; i < 8; i++) // bottom
        printf("---");
    printf("\n");
    fflush(stdout);
}

// takes piece at from and moves it to to
// piece at to is overwritten and from is set to EMPTY
void move_from_to(Piece* board, char* from, char* to)
{
    int row1, col1, row2, col2;

    row1 = from[1] - 48 - 1;
    col1 = from[0] - 96 - 1;
    Piece p = board[row1 * 8 + col1];
    board[row1 * 8 + col1] = EMPTY; // remove piece at from

    row2 = to[1] - 48 - 1;
    col2 = to[0] - 96 - 1;
    board[row2 * 8 + col2] = p; // put piece at to
}

// returns 1 if str is formated %c%d where char is [a-h] and d is [1-8], else 0
int is_valid_position(char* str)
{
    char letter = str[0];
    if (!(letter >= 97 && letter <= 104)) { // not a letter from a to h
        printf("Invalid Letter\n");
        return 0;
    }

    int number = str[1] - 48;
    if (!(number >= 1 && number <= 8)) { // not a number [1-8]
        printf("Invalid Number\n");
        return 0;
    }

    return 1;
}

// Returns a double array of Pieces at the starting position
// Side effect: mallocs, return must be freed by caller
Piece* init_board()
{
    Piece* board = malloc(sizeof(Piece) * 64);

    // make empty board
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            board[i * 8 + j] = EMPTY;
        }
    }

    // add pawns
    for (int i = 0; i < 8; i++) {
        board[8 + i] = WHITE_PAWN;
        board[8 * 6 + i] = BLACK_PAWN;
    }

    // add pieces
    for (int i = 0; i < 8; i++) {
        if (i == 0 || i == 7) {
            board[i] = WHITE_ROOK;
            board[7 * 8 + i] = BLACK_ROOK;
        }
        else if (i == 1 || i == 6) {
            board[i] = WHITE_KNIGHT;
            board[7 * 8 + i] = BLACK_KNIGHT;
        }
        else if (i == 2 || i == 5) {
            board[i] = WHITE_BISHOP;
            board[7 * 8 + i] = BLACK_BISHOP;
        }
        else if (i == 4) {
            board[i] = WHITE_KING;
            board[7 * 8 + i] = BLACK_KING;
        }
        else { // if (i == 4)
            board[i] = WHITE_QUEEN;
            board[7 * 8 + i] = BLACK_QUEEN;
        }
    }

    return board;
}

// outputs char value from piece
inline char piece_to_char(Piece piece)
{
    switch(piece) {
        case WHITE_PAWN:    return 'P';
        case WHITE_KNIGHT:  return 'N';
        case WHITE_BISHOP:  return 'P';
        case WHITE_ROOK:    return 'R';
        case WHITE_QUEEN:   return 'Q';
        case WHITE_KING:    return 'K';
        case BLACK_PAWN:    return 'p';
        case BLACK_KNIGHT:  return 'n';
        case BLACK_BISHOP:  return 'p';
        case BLACK_ROOK:    return 'r';
        case BLACK_QUEEN:   return 'q';
        case BLACK_KING:    return 'k';
        default:            return '-';
    }
}

// returns 1 if p is white, 0 otherwise
inline int is_white(Piece piece)
{
    if (piece == WHITE_PAWN || piece == WHITE_KNIGHT || piece == WHITE_BISHOP
        || piece == WHITE_ROOK || piece == WHITE_QUEEN || piece == WHITE_KING)
        return 1;
    return 0;
}

// returns 1 if p is black, 0 otherwise
inline int is_black(Piece piece)
{
    if (piece == BLACK_PAWN || piece == BLACK_KNIGHT || piece == BLACK_BISHOP
        || piece == BLACK_ROOK || piece == BLACK_QUEEN || piece == BLACK_KING)
        return 1;
    return 0;
}

inline void display_directions()
{
    printf("This chess program gives a starting board and allows for all\n");
    printf("moves, legal and illegal. To make a move, specify from and to.\n");
    printf("Moves must be formatted similar to \"a3\" where each move is a\n");
    printf("lowercase character from a-h and each number is from 1-8.\n");
    // printf("Press enter to continue\n");
}

inline void err_check(int val, int err, char* msg)
{
    if (val == err) {
        fprintf(stderr, "ERROR: %s\n", msg);
        exit(1);
    }
}
