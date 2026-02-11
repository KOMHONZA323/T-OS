#ifndef THEME_H
#define THEME_H


#include "../drivers/screen.h"

// T-OS Modern Dark Professional Theme

// Colors
#define THEME_BG            COLOR_BLACK
#define THEME_FG            COLOR_WHITE

// Top Bar (Fedora Style)
// "Slim, translucent black" -> Black BG
#define TOP_BAR_HEIGHT      1
#define TOP_BAR_ATTR        (COLOR_LIGHT_GREY | (COLOR_BLACK << 4))
#define TOP_BAR_CLOCK_ATTR  (COLOR_WHITE | (COLOR_BLACK << 4))
#define TOP_BAR_ACTIVE_ATTR (COLOR_WHITE | (COLOR_BLUE << 4)) // Highlight for active item

// Taskbar (Windows Hybrid)
// "Thicker, semi-transparent frosted glass taskbar" -> Approximated with Blue BG (Standard Windows feel)
// Using Blue (1) background to avoid blinking (background colors > 7 blink by default)
#define TASKBAR_HEIGHT      1
#define TASKBAR_ROW         (MAX_ROWS - 1)
#define TASKBAR_ATTR        (COLOR_WHITE | (COLOR_BLUE << 4))
#define START_BUTTON_ATTR   (COLOR_WHITE | (COLOR_CYAN << 4)) // Cyan BG for Start Button
#define ACTIVE_APP_ATTR     (COLOR_LIGHT_CYAN | (COLOR_BLUE << 4)) // Light Cyan text on Blue BG

// Windows
// "Rounded corners and subtle drop shadows" -> Rounded box characters
#define WINDOW_ATTR         (COLOR_LIGHT_GREY | (COLOR_BLACK << 4))
#define WINDOW_BORDER_ATTR  (COLOR_LIGHT_BLUE | (COLOR_BLACK << 4)) // Neon Blue border
#define WINDOW_TITLE_ATTR   (COLOR_WHITE | (COLOR_BLUE << 4)) // Blue Title Bar

// Wallpaper
// "Deep charcoal and obsidian abstract geometric wallpaper"
// Dark Grey foreground on Black background
#define WALLPAPER_ATTR      (COLOR_DARK_GREY | (COLOR_BLACK << 4))
#define WALLPAPER_CHAR      176 // Light shade block '░'

/* VGA Text Mode Colors (4-bit) */
#define BLACK 0x0
#define BLUE 0x1
#define GREEN 0x2
#define CYAN 0x3
#define RED 0x4
#define MAGENTA 0x5
#define BROWN 0x6
#define LIGHT_GREY 0x7
#define DARK_GREY 0x8
#define LIGHT_BLUE 0x9
#define LIGHT_GREEN 0xA
#define LIGHT_CYAN 0xB
#define LIGHT_RED 0xC
#define LIGHT_MAGENTA 0xD
#define YELLOW 0xE
#define WHITE 0xF

/* T-OS Design Specs Mapped to VGA */
/*
 * Primary BG (#0B0E14) -> BLACK
 * Surface (#161B22) -> BLACK (with DARK_GREY borders)
 * Accent (#58A6FF) -> LIGHT_BLUE or CYAN
 * Text (Main) (#C9D1D9) -> LIGHT_GREY
 * Text (Dim) (#8B949E) -> DARK_GREY
 */

/* Background Fill */
#define WALLPAPER_CHAR 0xB0 // Light shade
#define WALLPAPER_ATTR ((BLACK << 4) | DARK_GREY) // Dark Grey on Black

/* Top Bar (Translucent Black -> Black BG) */
#define TOP_BAR_BG ((BLACK << 4) | LIGHT_GREY)
#define TOP_BAR_TEXT_ATTR ((BLACK << 4) | WHITE)

/* Bottom Taskbar (Dark Grey -> Simulated with Block Char) */
/* We use Block Chars with DARK_GREY foreground and BLACK background to simulate Dark Grey surface */
#define TASKBAR_CHAR 0xDB // Full Block
#define TASKBAR_ATTR ((BLACK << 4) | DARK_GREY)
#define TASKBAR_TEXT_ATTR ((BLACK << 4) | WHITE) // Text must be on Black for readability if over block? No.
/* Text over block char is impossible. We must use Background Color.
 * VGA Background only 0-7. 0=Black, 8=DarkGrey (blinking).
 * So we are stuck with Black Background for text.
 * We can make the BAR black, but the ICONS colored.
 */
#define TASKBAR_BG_ATTR ((BLACK << 4) | LIGHT_GREY)

/* Windows */
#define WIN_BORDER_ATTR ((BLACK << 4) | DARK_GREY) // Deep Charcoal Border
#define WIN_TITLE_ATTR ((BLACK << 4) | WHITE)
#define WIN_CONTENT_ATTR ((BLACK << 4) | LIGHT_GREY) // Surface color
#define WIN_ACCENT_ATTR ((BLACK << 4) | LIGHT_BLUE)


#endif
