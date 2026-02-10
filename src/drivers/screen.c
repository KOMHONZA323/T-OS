#include "screen.h"
#include "ports.h"
#include "utils.h"

/* Declaration of private functions */
int get_cursor_offset();
void set_cursor_offset(int offset);
int print_char(char c, int col, int row, char attr);
int get_offset(int col, int row);
int handle_scrolling(int cursor_offset);

/**********************************************************
 * Public Kernel API functions                            *
 **********************************************************/

/**
 * Print a message on the specified location
 * If col, row, are negative, we will use the current offset
 */
void kprint_at_attr(char *message, int col, int row, char attr) {
    /* Set cursor if col/row are negative */
    int offset;
    if (col >= 0 && row >= 0)
        offset = get_offset(col, row);
    else {
        offset = get_cursor_offset();
        row = offset / (2 * MAX_COLS);
        col = (offset - (row * 2 * MAX_COLS)) / 2;
    }

    int i = 0;
    while (message[i] != 0) {
        offset = print_char(message[i++], col, row, attr);
        /* Compute row/col for next char */
        row = offset / (2 * MAX_COLS);
        col = (offset - (row * 2 * MAX_COLS)) / 2;
    }
}

void kprint_at(char *message, int col, int row) {
    kprint_at_attr(message, col, row, WHITE_ON_BLACK);
}

void kprint(char *message) {
    kprint_at(message, -1, -1);
}

void kprint_backspace() {
    int offset = get_cursor_offset() - 2;
    int row = offset / (2 * MAX_COLS);
    int col = (offset - (row * 2 * MAX_COLS)) / 2;
    print_char(0x08, col, row, WHITE_ON_BLACK);
}

/**********************************************************
 * Private kernel functions                               *
 **********************************************************/

/**
 * Innermost print function for our kernel, directly accesses the video memory
 *
 * If 'col' and 'row' are negative, we will print at current cursor location
 * If 'attr' is zero it will use 'white on black' as default
 * Returns the offset of the next character
 * Sets the video cursor to the returned offset
 */
int print_char(char c, int col, int row, char attr) {
    unsigned char *vidmem = (unsigned char*) VIDEO_ADDRESS;
    if (!attr) attr = WHITE_ON_BLACK;

    /* Error control: print a red 'E' if the coords aren't right */
    if (col >= MAX_COLS || row >= MAX_ROWS) {
        vidmem[2*(MAX_COLS*MAX_ROWS)-2] = 'E';
        vidmem[2*(MAX_COLS*MAX_ROWS)-1] = RED_ON_WHITE;
        return get_offset(col, row);
    }

    int offset;
    if (col >= 0 && row >= 0) offset = get_offset(col, row);
    else offset = get_cursor_offset();

    if (c == '\n') {
        row = offset / (2 * MAX_COLS);
        offset = get_offset(0, row + 1);
    } else if (c == 0x08) { /* Backspace */
        vidmem[offset] = ' ';
        vidmem[offset+1] = attr;
    } else {
        vidmem[offset] = c;
        vidmem[offset+1] = attr;
        offset += 2;
    }

    /* Check if the offset is over screen size and scroll */
    offset = handle_scrolling(offset);
    set_cursor_offset(offset);
    return offset;
}

int get_cursor_offset() {
    /* Use the VGA ports to get the current cursor position
     * 1. Ask for high byte of the cursor offset (data 14)
     * 2. Ask for low byte (data 15)
     */
    port_byte_out(REG_SCREEN_CTRL, 14);
    int offset = port_byte_in(REG_SCREEN_DATA) << 8;
    port_byte_out(REG_SCREEN_CTRL, 15);
    offset += port_byte_in(REG_SCREEN_DATA);
    return offset * 2; /* Position * size of character cell */
}

void set_cursor_offset(int offset) {
    /* Similar to get_cursor_offset, but writing data */
    offset /= 2;
    port_byte_out(REG_SCREEN_CTRL, 14);
    port_byte_out(REG_SCREEN_DATA, (unsigned char)(offset >> 8));
    port_byte_out(REG_SCREEN_CTRL, 15);
    port_byte_out(REG_SCREEN_DATA, (unsigned char)(offset & 0xff));
}

void clear_screen() {
    int screen_size = MAX_COLS * MAX_ROWS;
    int i;
    unsigned char *screen = (unsigned char*) VIDEO_ADDRESS;

    for (i = 0; i < screen_size; i++) {
        screen[i*2] = ' ';
        screen[i*2+1] = WHITE_ON_BLACK;
    }
    set_cursor_offset(get_offset(0, 0));
}


int get_offset(int col, int row) { return 2 * (row * MAX_COLS + col); }

int handle_scrolling(int cursor_offset) {
    if (cursor_offset < MAX_ROWS * MAX_COLS * 2) return cursor_offset;

    /* Shuffle the rows back one */
    int i;
    for (i = 1; i < MAX_ROWS; i++) {
        memory_copy((char*)(get_offset(0, i) + VIDEO_ADDRESS),
                    (char*)(get_offset(0, i-1) + VIDEO_ADDRESS),
                    MAX_COLS * 2);
    }

    /* Blank the last line */
    char *last_line = (char*)(get_offset(0, MAX_ROWS-1) + VIDEO_ADDRESS);
    for (i = 0; i < MAX_COLS * 2; i++) last_line[i] = 0;

    cursor_offset -= 2 * MAX_COLS;
    return cursor_offset;
}

/* TUI Extension functions */

void draw_rect(int col, int row, int width, int height, char attr) {
    unsigned char *vidmem = (unsigned char*) VIDEO_ADDRESS;
    for (int r = row; r < row + height; r++) {
        for (int c = col; c < col + width; c++) {
            if (c >= MAX_COLS || r >= MAX_ROWS) continue;
            int offset = get_offset(c, r);
            vidmem[offset] = ' '; // Clear char
            vidmem[offset+1] = attr;
        }
    }
}

void draw_fill(int col, int row, int width, int height, char ch, char attr) {
    unsigned char *vidmem = (unsigned char*) VIDEO_ADDRESS;
    for (int r = row; r < row + height; r++) {
        for (int c = col; c < col + width; c++) {
            if (c >= MAX_COLS || r >= MAX_ROWS) continue;
            int offset = get_offset(c, r);
            vidmem[offset] = ch;
            vidmem[offset+1] = attr;
        }
    }
}

void draw_box(int col, int row, int width, int height, char border_attr, char inner_attr) {
    unsigned char *vidmem = (unsigned char*) VIDEO_ADDRESS;

    // Draw content
    draw_rect(col + 1, row + 1, width - 2, height - 2, inner_attr);

    // Draw borders
    // Top and Bottom
    for (int c = col; c < col + width; c++) {
        int off_top = get_offset(c, row);
        int off_bot = get_offset(c, row + height - 1);
        vidmem[off_top] = 205; // Double horizontal line
        vidmem[off_top+1] = border_attr;
        vidmem[off_bot] = 205;
        vidmem[off_bot+1] = border_attr;
    }

    // Left and Right
    for (int r = row; r < row + height; r++) {
        int off_left = get_offset(col, r);
        int off_right = get_offset(col + width - 1, r);
        vidmem[off_left] = 186; // Double vertical line
        vidmem[off_left+1] = border_attr;
        vidmem[off_right] = 186;
        vidmem[off_right+1] = border_attr;
    }

    // Corners
    int off_tl = get_offset(col, row);
    int off_tr = get_offset(col + width - 1, row);
    int off_bl = get_offset(col, row + height - 1);
    int off_br = get_offset(col + width - 1, row + height - 1);

    vidmem[off_tl] = 201; // Top-left
    vidmem[off_tl+1] = border_attr;
    vidmem[off_tr] = 187; // Top-right
    vidmem[off_tr+1] = border_attr;
    vidmem[off_bl] = 200; // Bottom-left
    vidmem[off_bl+1] = border_attr;
    vidmem[off_br] = 188; // Bottom-right
    vidmem[off_br+1] = border_attr;
}

/* Rounded Box Implementation using CP437 Single Lines */
void draw_box_rounded(int col, int row, int width, int height, char border_attr, char inner_attr, char title_attr) {
    unsigned char *vidmem = (unsigned char*) VIDEO_ADDRESS;

    // 1. Fill Content (Inner)
    draw_fill(col + 1, row + 1, width - 2, height - 2, ' ', inner_attr);

    // 2. Borders (Single Lines for Sleek Look)
    // Top and Bottom
    for (int c = col + 1; c < col + width - 1; c++) {
        int off_top = get_offset(c, row);
        int off_bot = get_offset(c, row + height - 1);
        vidmem[off_top] = 196; // Single horizontal (─)
        vidmem[off_top+1] = border_attr;
        vidmem[off_bot] = 196;
        vidmem[off_bot+1] = border_attr;
    }

    // Left and Right
    for (int r = row + 1; r < row + height - 1; r++) {
        int off_left = get_offset(col, r);
        int off_right = get_offset(col + width - 1, r);
        vidmem[off_left] = 179; // Single vertical (│)
        vidmem[off_left+1] = border_attr;
        vidmem[off_right] = 179;
        vidmem[off_right+1] = border_attr;
    }

    // 3. Corners (Single Round/Arc)
    int off_tl = get_offset(col, row);
    int off_tr = get_offset(col + width - 1, row);
    int off_bl = get_offset(col, row + height - 1);
    int off_br = get_offset(col + width - 1, row + height - 1);

    // CP437 does not have true rounded corners, using single line corners (DA, BF, C0, D9)
    // ┌ (218) ┐ (191)
    // └ (192) ┘ (217)
    vidmem[off_tl] = 218;
    vidmem[off_tl+1] = border_attr;
    vidmem[off_tr] = 191;
    vidmem[off_tr+1] = border_attr;
    vidmem[off_bl] = 192;
    vidmem[off_bl+1] = border_attr;
    vidmem[off_br] = 217;
    vidmem[off_br+1] = border_attr;

    // 4. Title Bar Background (Optional)
    // If title_attr is different, draw a filled rectangle for the title row
    if (title_attr != inner_attr) {
        for (int c = col + 1; c < col + width - 1; c++) {
             int off_title = get_offset(c, row); // Overwrite top border? No, inside.
             // Actually, usually title bar is row+1? Or replaces top border?
             // Let's make title bar at row, effectively replacing top border? No.
             // Title bar inside the box at row+1
             int off_t = get_offset(c, row + 1);
             // We can fill this row
             // vidmem[off_t] = ' ';
             // vidmem[off_t+1] = title_attr;
        }
    }
}
