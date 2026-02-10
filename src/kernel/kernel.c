/*
 * T-OS Minimal Kernel
 * Freestanding C Environment - Cursor Logo Display
 */

#define VIDEO_MEMORY 0xb8000
#define COLS 80
#define ROWS 25
#define BYTES_PER_CELL 2
#define ROW_SIZE (COLS * BYTES_PER_CELL)

#define WHITE_ON_BLACK 0x0f
#define BRIGHT_WHITE 0x0f
#define LIGHT_CYAN 0x0b

/* VGA block characters */
#define BLOCK_FULL  0xDB
#define BLOCK_UPPER 0xDC
#define BLOCK_LOWER 0xDF

void clear_screen(void);
void put_char_at(int row, int col, char c, unsigned char attr);
void print_at(int row, int col, const char* msg);
void draw_cursor_logo(void);

void kernel_main(void) {
    clear_screen();
    draw_cursor_logo();
    print_at(14, 33, "T-OS");
    print_at(16, 28, "Powered by Cursor");

    while (1)
        ;
}

void clear_screen(void) {
    unsigned char* vid = (unsigned char*) VIDEO_MEMORY;
    for (int i = 0; i < COLS * ROWS * BYTES_PER_CELL; i += 2) {
        vid[i] = ' ';
        vid[i + 1] = WHITE_ON_BLACK;
    }
}

void put_char_at(int row, int col, char c, unsigned char attr) {
    if (row < 0 || row >= ROWS || col < 0 || col >= COLS)
        return;
    unsigned char* vid = (unsigned char*) VIDEO_MEMORY;
    int offset = (row * ROW_SIZE) + (col * BYTES_PER_CELL);
    vid[offset] = c;
    vid[offset + 1] = attr;
}

void print_at(int row, int col, const char* msg) {
    while (*msg) {
        put_char_at(row, col, *msg, WHITE_ON_BLACK);
        msg++;
        col++;
    }
}

/* Cursor IDE-style logo: vertical bar with arrow (7x7) */
void draw_cursor_logo(void) {
    const unsigned char attr = LIGHT_CYAN;
    int start_row = 6;
    int start_col = 36;

    /* Top cap */
    put_char_at(start_row + 0, start_col + 3, BLOCK_UPPER, attr);

    /* Vertical bar */
    put_char_at(start_row + 1, start_col + 3, BLOCK_FULL, attr);
    put_char_at(start_row + 2, start_col + 3, BLOCK_FULL, attr);
    put_char_at(start_row + 3, start_col + 3, BLOCK_FULL, attr);
    put_char_at(start_row + 4, start_col + 3, BLOCK_FULL, attr);

    /* Arrow head (bottom) */
    put_char_at(start_row + 5, start_col + 1, BLOCK_LOWER, attr);
    put_char_at(start_row + 5, start_col + 2, BLOCK_LOWER, attr);
    put_char_at(start_row + 5, start_col + 3, BLOCK_LOWER, attr);
    put_char_at(start_row + 5, start_col + 4, BLOCK_LOWER, attr);
    put_char_at(start_row + 5, start_col + 5, BLOCK_LOWER, attr);
}
