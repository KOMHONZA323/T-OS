#ifndef SCREEN_H
#define SCREEN_H

#define VIDEO_ADDRESS 0xb8000
#define MAX_ROWS 25
#define MAX_COLS 80
#define WHITE_ON_BLACK 0x0f
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
