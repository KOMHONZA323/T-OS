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

// Background: Deep Charcoal (#1E1E1E), Obsidian (#0B0B0B)
#define COLOR_OBSIDIAN 0xFF0B0E14
#define COLOR_CHARCOAL 0xFF161B22
#define COLOR_DEEP_BG  0xFF0D1117

// Accents: Neon Blue (#00FFFF or similar) -> Using a nice bright blue
#define COLOR_NEON_BLUE 0xFF58A6FF
#define COLOR_CYAN_ACCENT 0xFF39D353 // GitHub Green-ish accent for success
#define COLOR_RED_ACCENT 0xFFFF7B72 // Error red

// Text
#define COLOR_TEXT_MAIN 0xFFC9D1D9
#define COLOR_TEXT_DIM  0xFF8B949E

// UI Components
// Top Bar: Translucent Black
#define COLOR_TOP_BAR_BG 0xCC000000 // Alpha ~80%
#define COLOR_TOP_BAR_TEXT COLOR_TEXT_MAIN

// Taskbar: Semi-transparent frosted
#define COLOR_TASKBAR_BG 0xAA161B22 // Alpha ~66%
#define COLOR_TASKBAR_ICON_ACTIVE COLOR_NEON_BLUE
#define COLOR_TASKBAR_ICON_INACTIVE COLOR_TEXT_DIM

// Windows
#define COLOR_WIN_BG COLOR_CHARCOAL
#define COLOR_WIN_BORDER COLOR_TEXT_DIM // Dark Grey
#define COLOR_WIN_TITLE COLOR_TEXT_MAIN
#define COLOR_WIN_SHADOW 0x80000000 // 50% Black Shadow

/* Legacy Mappings (still used by text mode logic if needed) */
#define WALLPAPER_CHAR 0xB0
#define WALLPAPER_ATTR ((BLACK << 4) | DARK_GREY)

#define TOP_BAR_BG ((BLACK << 4) | LIGHT_GREY)
#define TOP_BAR_TEXT_ATTR ((BLACK << 4) | WHITE)

#define TASKBAR_CHAR 0xDB
#define TASKBAR_ATTR ((BLACK << 4) | DARK_GREY)
#define TASKBAR_TEXT_ATTR ((BLACK << 4) | WHITE)
#define TASKBAR_BG_ATTR ((BLACK << 4) | LIGHT_GREY)

#define WIN_BORDER_ATTR ((BLACK << 4) | DARK_GREY)
#define WIN_TITLE_ATTR ((BLACK << 4) | WHITE)
#define WIN_CONTENT_ATTR ((BLACK << 4) | LIGHT_GREY)
#define WIN_ACCENT_ATTR ((BLACK << 4) | LIGHT_BLUE)

#endif
