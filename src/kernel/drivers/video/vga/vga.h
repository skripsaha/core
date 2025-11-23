#ifndef VGA_H
#define VGA_H

#include "ktypes.h"

// ============================================================================
// VGA TEXT MODE DRIVER - 80x25 Color Display
// ============================================================================

#define VGA_MEMORY 0xB8000
extern unsigned char *vga;

// Screen dimensions
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define BYTES_FOR_EACH_ELEMENT 2
#define VGA_SIZE (VGA_WIDTH * VGA_HEIGHT * BYTES_FOR_EACH_ELEMENT)

// ============================================================================
// VGA COLORS (4-bit)
// ============================================================================
// Foreground (text) colors: bits 0-3
// Background colors: bits 4-6 (bit 7 = blink if enabled)

#define VGA_BLACK         0x0
#define VGA_BLUE          0x1
#define VGA_GREEN         0x2
#define VGA_CYAN          0x3
#define VGA_RED           0x4
#define VGA_MAGENTA       0x5
#define VGA_BROWN         0x6
#define VGA_LIGHT_GRAY    0x7
#define VGA_DARK_GRAY     0x8
#define VGA_LIGHT_BLUE    0x9
#define VGA_LIGHT_GREEN   0xA
#define VGA_LIGHT_CYAN    0xB
#define VGA_LIGHT_RED     0xC
#define VGA_LIGHT_MAGENTA 0xD
#define VGA_YELLOW        0xE
#define VGA_WHITE         0xF

// Create attribute byte: VGA_ATTR(foreground, background)
#define VGA_ATTR(fg, bg) ((uint8_t)(((bg) << 4) | (fg)))

// ============================================================================
// PREDEFINED COLOR SCHEMES FOR SHELL
// ============================================================================

// Standard colors (light gray on black)
#define VGA_DEFAULT       VGA_ATTR(VGA_LIGHT_GRAY, VGA_BLACK)   // 0x07

// Semantic colors
#define VGA_ERROR         VGA_ATTR(VGA_LIGHT_RED, VGA_BLACK)    // 0x0C - Errors
#define VGA_SUCCESS       VGA_ATTR(VGA_LIGHT_GREEN, VGA_BLACK)  // 0x0A - Success
#define VGA_WARNING       VGA_ATTR(VGA_YELLOW, VGA_BLACK)       // 0x0E - Warnings
#define VGA_HINT          VGA_ATTR(VGA_LIGHT_CYAN, VGA_BLACK)   // 0x0B - Hints/info
#define VGA_CURSOR        VGA_ATTR(VGA_LIGHT_BLUE, VGA_BLACK)   // 0x09 - Cursor

// Shell prompt colors
#define VGA_PROMPT        VGA_ATTR(VGA_LIGHT_GREEN, VGA_BLACK)  // 0x0A - ~
#define VGA_PROMPT_TAG    VGA_ATTR(VGA_CYAN, VGA_BLACK)         // 0x03 - [tag:value]
#define VGA_INPUT         VGA_ATTR(VGA_WHITE, VGA_BLACK)        // 0x0F - User input

// File types
#define VGA_FILE          VGA_ATTR(VGA_WHITE, VGA_BLACK)        // 0x0F - Regular file
#define VGA_DIRECTORY     VGA_ATTR(VGA_LIGHT_BLUE, VGA_BLACK)   // 0x09 - Directory
#define VGA_EXECUTABLE    VGA_ATTR(VGA_LIGHT_GREEN, VGA_BLACK)  // 0x0A - Executable
#define VGA_SPECIAL       VGA_ATTR(VGA_YELLOW, VGA_BLACK)       // 0x0E - Special file

// System messages
#define VGA_KERNEL        VGA_ATTR(VGA_LIGHT_MAGENTA, VGA_BLACK) // 0x0D - Kernel msgs
#define VGA_DEBUG         VGA_ATTR(VGA_DARK_GRAY, VGA_BLACK)    // 0x08 - Debug info

// Highlight colors (for selection, etc.)
#define VGA_HIGHLIGHT     VGA_ATTR(VGA_BLACK, VGA_LIGHT_GRAY)   // 0x70 - Inverted
#define VGA_SELECTED      VGA_ATTR(VGA_BLACK, VGA_CYAN)         // 0x30 - Selected item

// Legacy aliases (for compatibility)
#define TEXT_ATTR_DEFAULT    VGA_DEFAULT
#define TEXT_ATTR_CURSOR     VGA_CURSOR
#define TEXT_ATTR_ERROR      VGA_ERROR
#define TEXT_ATTR_WARNING    VGA_WARNING
#define TEXT_ATTR_HINT       VGA_HINT
#define TEXT_ATTR_SUCCESS    VGA_SUCCESS

// ============================================================================
// VGA API FUNCTIONS
// ============================================================================

// Initialization
void vga_init(void);

// Basic output
void vga_print(const char *str);
void vga_print_attr(const char *str, uint8_t attr);  // Print with custom attribute
void vga_print_char(char ch, uint8_t attr);
void vga_print_newline(void);

// Screen control
void vga_clear_screen(void);
void vga_clear_line(int line);
void vga_clear_to_eol(void);
void vga_scroll_up(void);
void vga_change_background(uint8_t attr);

// Semantic output (convenience functions)
void vga_print_error(const char *str);
void vga_print_success(const char *str);
void vga_print_hint(const char *str);
void vga_print_warning(const char *str);

// Cursor control
void vga_update_cursor(void);
void vga_set_cursor_position(int x, int y);
int vga_get_cursor_position_x(void);
int vga_get_cursor_position_y(void);

// Get current location (for shell line editing)
unsigned int vga_get_current_loc(void);
void vga_set_current_loc(unsigned int loc);

#endif // VGA_H