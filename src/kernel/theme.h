#ifndef THEME_H
#define THEME_H

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

/* Modern Dark Professional Theme (ARGB 8888) */
#define COLOR_OBSIDIAN 0xFF121212
#define COLOR_CHARCOAL 0xFF1E1E1E
#define COLOR_NEON_BLUE 0xFF00E5FF
#define COLOR_GLASS_WHITE 0x40FFFFFF
#define COLOR_TOP_BAR_BG 0xD0000000 // Translucent Black
#define COLOR_TASKBAR_BG 0xB01E1E1E // Semi-transparent Frosted Glass
#define COLOR_TOP_BAR_TEXT 0xFFFFFFFF

#define COLOR_ICON_GLOW 0x4000E5FF
#define COLOR_SHADOW 0x60000000
#define COLOR_GLASS_ACCENT 0x20FFFFFF
#define COLOR_WIFI 0xFFFFFFFF
#define COLOR_BATTERY 0xFFFFFFFF

#endif
