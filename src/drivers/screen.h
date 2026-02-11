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
=======
#define RED_ON_WHITE 0xf4

/* Screen i/o ports */
#define REG_SCREEN_CTRL 0x3d4
#define REG_SCREEN_DATA 0x3d5

/* Public kernel API */
void clear_screen();
void kprint_at(char *message, int col, int row);
void kprint_at_attr(char *message, int col, int row, char attr);
void kprint(char *message);
void kprint_backspace();
void draw_rect(int col, int row, int width, int height, char attr);
void draw_box(int col, int row, int width, int height, char border_attr, char inner_attr);

/* T-OS Specific */
void draw_fill(int col, int row, int width, int height, char c, char attr);
void draw_box_rounded(int col, int row, int width, int height, char border_attr, char inner_attr, char title_attr);


#endif
