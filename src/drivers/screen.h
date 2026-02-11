#ifndef SCREEN_H
#define SCREEN_H

#define VIDEO_ADDRESS 0xb8000
#define MAX_ROWS 25
#define MAX_COLS 80
#define WHITE_ON_BLACK 0x0f

// Colors (foreground | background << 4)
#define COLOR_BLACK 0x0
#define COLOR_BLUE 0x1
#define COLOR_GREEN 0x2
#define COLOR_CYAN 0x3
#define COLOR_RED 0x4
#define COLOR_MAGENTA 0x5
#define COLOR_BROWN 0x6
#define COLOR_LIGHT_GREY 0x7
#define COLOR_DARK_GREY 0x8
#define COLOR_LIGHT_BLUE 0x9
#define COLOR_LIGHT_GREEN 0xA
#define COLOR_LIGHT_CYAN 0xB
#define COLOR_LIGHT_RED 0xC
#define COLOR_LIGHT_MAGENTA 0xD
#define COLOR_YELLOW 0xE
#define COLOR_WHITE 0xF

void clear_screen();
void put_char(char c, int row, int col, int attr);
void print_at(const char* message, int row, int col, int attr);
void draw_rect(int row, int col, int width, int height, int attr, char c);
void fill_rect(int row, int col, int width, int height, int attr, char c);
void draw_box(int row, int col, int width, int height, int attr);
void print_centered(const char* message, int row, int attr);
void print_right_aligned(const char* message, int row, int attr);

#endif
