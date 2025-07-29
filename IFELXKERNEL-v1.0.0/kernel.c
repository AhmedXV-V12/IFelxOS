#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "vga.h"
#include "keyboard.h"
#include "commands.h"

void kernel_main(void) {
    // Initialize hardware and display
    vga_init();
    vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLUE);
    vga_clear();

    vga_printf("IFELXOS\n");
    vga_printf("Type 'help' for commands\n\n");
    
    // Main command loop
    char input[256];
    while (1) {
        // Display prompt
        vga_printf("IFELX~$ ");
        
        // Read user input
        keyboard_readline(input, sizeof(input) - 1);
        input[sizeof(input) - 1] = '\0'; // Ensure null termination
        
        // Handle command
        vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLUE);
        if (!commands_handle(input)) {
            vga_printf(" Erorr xv2b2+7000  Unknown command. Type 'help'\n");
        }
    }
}
