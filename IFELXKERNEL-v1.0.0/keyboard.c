#include "keyboard.h"
#include "vga.h"
#include <stdint.h>
#include <stdbool.h>

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// scancode to ascii (US QWERTY), index = scancode, shift=0 or 1
static const char scancode_table[2][128] = {
    // no shift
    {
        0,    27,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 8,  9,
        'q',  'w',  'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\\', 0, 'a', 's',
        'd',  'f',  'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,   '#', 'z', 'x', 'c', 'v',
        'b',  'n',  'm', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
        0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
    },
    // shift
    {
        0,    27,   '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 8,  9,
        'Q',  'W',  'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '|',  0, 'A', 'S',
        'D',  'F',  'G', 'H', 'J', 'K', 'L', ':', '\"', '~', 0,   '~', 'Z', 'X', 'C', 'V',
        'B',  'N',  'M', '<', '>', '?', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
        0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        0,    0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
    }
};

void keyboard_readline(char* buf, size_t maxlen) {
    if (!buf || maxlen == 0) return;
    
    size_t i = 0;
    static bool shift = false;
    
    // Initialize buffer
    buf[0] = '\0';
    
    while (i + 1 < maxlen) {
        // Wait for data to be available
        while ((inb(0x64) & 0x01) == 0);
        
        uint8_t sc = inb(0x60);
        
        // Skip empty scancodes
        if (sc == 0) continue;
        
        // Check if key is pressed or released
        bool key_pressed = !(sc & 0x80);
        sc &= 0x7F; // Remove the release bit
        
        // Handle modifier keys
        switch (sc) {
            case 0x2A:  // Left Shift
            case 0x36:  // Right Shift
                shift = key_pressed;
                break;
            default:
                // Only process key press events for non-modifier keys
                if (key_pressed) {
                    // Handle special keys
                    if (sc == 0x0E) {  // Backspace
                        if (i > 0) {
                            i--;
                            vga_putc('\b');
                            vga_putc(' ');
                            vga_putc('\b');
                            buf[i] = '\0';
                        }
                        continue;
                    } else if (sc == 0x1C) {  // Enter
                        vga_putc('\n');
                        buf[i] = '\0';
                        return;
                    } else if (sc < 128) {  // Regular key
                        if (i + 1 < maxlen) {  // Leave space for null terminator
                            char c = scancode_table[shift ? 1 : 0][sc];
                            if (c >= 32 && c <= 126) {  // Printable ASCII
                                buf[i++] = c;
                                buf[i] = '\0';  // Keep string null-terminated
                                vga_putc(c);
                            }
                        }
                    }
                }
                break;
        }
    }
    
    // Null-terminate the string and print newline
    buf[i] = '\0';
    vga_putc('\n');
    return;
}

bool keyboard_check_esc(void) {
    uint8_t status = inb(0x64);
    if (status & 0x01) { // إذا كان هناك بيانات متاحة
        uint8_t scancode = inb(0x60);
        return (scancode == 0x01); // 0x01 هو scancode مفتاح ESC
    }
    return false;
}
