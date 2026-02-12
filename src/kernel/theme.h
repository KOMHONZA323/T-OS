#ifndef THEME_H
#define THEME_H

/* VGA Text Mode Colors (4-bit) - Legacy */
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

/* Modern T-OS Design Specs (ARGB 8888) */
// "Modern Dark Professional"

/* Background Fill */
#define WALLPAPER_CHAR ' ' // Clear
#define WALLPAPER_ATTR ((BLACK << 4) | BLACK) // Solid Black

#define TOP_BAR_BG ((BLACK << 4) | LIGHT_GREY)
#define TOP_BAR_TEXT_ATTR ((BLACK << 4) | WHITE)

#define TASKBAR_CHAR 0xDB
#define TASKBAR_ATTR ((BLACK << 4) | DARK_GREY)
#define TASKBAR_TEXT_ATTR ((BLACK << 4) | WHITE)
#define TASKBAR_BG_ATTR ((BLACK << 4) | LIGHT_GREY)

/* Windows */
#define WIN_BORDER_ATTR ((DARK_GREY << 4) | LIGHT_GREY) // Light Grey on Dark Grey
#define WIN_TITLE_ATTR ((DARK_GREY << 4) | WHITE)
#define WIN_CONTENT_ATTR ((DARK_GREY << 4) | LIGHT_GREY) // Solid Dark Grey BG
#define WIN_ACCENT_ATTR ((DARK_GREY << 4) | LIGHT_CYAN)

#endif
