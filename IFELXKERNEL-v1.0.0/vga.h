#ifndef VGA_H
#define VGA_H
#include <stdint.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

enum vga_color {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14,
    VGA_COLOR_WHITE = 15
};

void vga_init();
void vga_setcolor(uint8_t fg, uint8_t bg);
void vga_clear();
void vga_putc(char c);
void vga_puts(const char* str);
void vga_printf(const char* format, ...);
void vga_init_graphics(void);
void vga_init_text(void);
void vga_draw_rect(int x, int y, int width, int height, uint8_t color);
void vga_set_graphics_mode();
void vga_set_pixel(int x, int y, uint8_t color);
void vga_clear_graphics();
void vga_putn(int n);

#endif
