#include "screen.h"
#include "utils.h"

int get_screen_offset(int row, int col) {
    return 2 * (row * MAX_COLS + col);
}

void put_char(char c, int row, int col, int attr) {
    if (row >= MAX_ROWS || col >= MAX_COLS || row < 0 || col < 0) return;
    unsigned char* vidmem = (unsigned char*) VIDEO_ADDRESS;
    int offset = get_screen_offset(row, col);
    vidmem[offset] = c;
    vidmem[offset+1] = attr;
}

void print_at(const char* message, int row, int col, int attr) {
    int i = 0;
    while (message[i] != 0) {
        put_char(message[i++], row, col++, attr);
    }
}

void clear_screen() {
    int row, col;
    for (row = 0; row < MAX_ROWS; row++) {
        for (col = 0; col < MAX_COLS; col++) {
            put_char(' ', row, col, WHITE_ON_BLACK);
        }
    }
}

void draw_rect(int row, int col, int width, int height, int attr, char c) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            put_char(c, row + i, col + j, attr);
        }
    }
}

void fill_rect(int row, int col, int width, int height, int attr, char c) {
    draw_rect(row, col, width, height, attr, c);
}

void draw_box(int row, int col, int width, int height, int attr) {
    // Top
    for (int i=0; i<width; i++) put_char(0xCD, row, col+i, attr);
    // Bottom
    for (int i=0; i<width; i++) put_char(0xCD, row+height-1, col+i, attr);
    // Sides
    for (int i=0; i<height; i++) {
        put_char(0xBA, row+i, col, attr);
        put_char(0xBA, row+i, col+width-1, attr);
    }
    // Corners
    put_char(0xC9, row, col, attr); // TL
    put_char(0xBB, row, col+width-1, attr); // TR
    put_char(0xC8, row+height-1, col, attr); // BL
    put_char(0xBC, row+height-1, col+width-1, attr); // BR
}

void print_centered(const char* message, int row, int attr) {
    int len = strlen(message);
    int col = (MAX_COLS - len) / 2;
    print_at(message, row, col, attr);
}

void print_right_aligned(const char* message, int row, int attr) {
    int len = strlen(message);
    int col = MAX_COLS - len - 1;
    print_at(message, row, col, attr);
}
