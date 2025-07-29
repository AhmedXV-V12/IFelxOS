#include "vga.h"
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include "mini_string.h"

static uint16_t* const VGA_MEMORY = (uint16_t*)0xB8000;
static uint8_t vga_color = 0x1F;
static size_t vga_row = 0, vga_col = 0;

static inline uint8_t vga_entry_color(uint8_t fg, uint8_t bg) {
    return fg | bg << 4;
}

void vga_init() {
    vga_row = 0;
    vga_col = 0;
    vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLUE);
    vga_clear();
}

void vga_setcolor(uint8_t fg, uint8_t bg) {
    vga_color = vga_entry_color(fg, bg);
}

void vga_clear() {
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            VGA_MEMORY[y * VGA_WIDTH + x] = (uint16_t)(' ' | (vga_color << 8));
        }
    }
    vga_row = 0;
    vga_col = 0;
}

void vga_puts(const char* str) {
    while (*str) {
        vga_putc(*str++);
    }
}

void vga_putc(char c) {
    switch (c) {
        case '\n': // سطر جديد
            vga_row++;
            vga_col = 0;
            break;
            
        case '\b': // مسح للخلف
            if (vga_col > 0) {
                vga_col--;
                // مسح الحرف الحالي
                VGA_MEMORY[vga_row * VGA_WIDTH + vga_col] = (uint16_t)(' ' | (vga_color << 8));
            } else if (vga_row > 0) {
                // الانتقال إلى نهاية السطر السابق
                vga_row--;
                vga_col = VGA_WIDTH - 1;
                VGA_MEMORY[vga_row * VGA_WIDTH + vga_col] = (uint16_t)(' ' | (vga_color << 8));
            }
            break;
            
        default: // حرف عادي
            VGA_MEMORY[vga_row * VGA_WIDTH + vga_col] = (uint16_t)(c | (vga_color << 8));
            vga_col++;
            if (vga_col == VGA_WIDTH) {
                vga_col = 0;
                vga_row++;
            }
            break;
    }
    
    // التمرير للأسفل إذا لزم الأمر
    if (vga_row == VGA_HEIGHT) {
        vga_clear();
    }
}

void vga_printf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    
    char* ptr = buffer;
    while (*ptr) {
        vga_putc(*ptr++);
    }
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

void vga_set_graphics_mode() {
    // Set VGA mode 13h (320x200, 256 colors)
    __asm__ volatile ("int $0x10" : : "a"(0x0013));
}

void vga_set_pixel(int x, int y, uint8_t color) {
    if (x >= 0 && x < 320 && y >= 0 && y < 200) {
        uint8_t* vga_mem = (uint8_t*)0xA0000;
        vga_mem[y * 320 + x] = color;
    }
}

void vga_clear_graphics() {
    for (int y = 0; y < 200; y++) {
        for (int x = 0; x < 320; x++) {
            vga_set_pixel(x, y, 0);
        }
    }
}

void vga_putn(int n) {
    if (n == 0) {
        vga_putc('0');
        return;
    }

    // Handle negative numbers
    if (n < 0) {
        vga_putc('-');
        n = -n;
    }

    // Convert number to string in reverse order
    char buffer[16];
    int i = 0;
    
    while (n > 0) {
        buffer[i++] = '0' + (n % 10);
        n /= 10;
    }
    
    // Print in correct order
    while (--i >= 0) {
        vga_putc(buffer[i]);
    }
}

// تهيئة وضع الرسوميات (VGA Mode 13h - 320x200 256 colors)
void vga_init_graphics(void) {
    // تفعيل وضع الرسوميات 320x200x256
    vga_set_graphics_mode();
    vga_clear_graphics();
}

// العودة إلى وضع النص (80x25)
void vga_init_text(void) {
    // تفعيل وضع النص 80x25
    outb(0x3D4, 0x00);
    outb(0x3D5, 0x0F);
    outb(0x3D4, 0x01);
    outb(0x3D5, 0x00);
    outb(0x3D4, 0x03);
    outb(0x3D5, 0x00);
    outb(0x3D4, 0x05);
    outb(0x3D5, 0x00);
    outb(0x3D4, 0x07);
    outb(0x3D5, 0x00);
    
    // إعادة تهيئة VGA
    vga_init();
}

// رسم مستطيل
void vga_draw_rect(int x, int y, int width, int height, uint8_t color) {
    for (int dy = 0; dy < height; dy++) {
        for (int dx = 0; dx < width; dx++) {
            vga_set_pixel(x + dx, y + dy, color);
        }
    }
}
