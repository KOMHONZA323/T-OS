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

#endif
