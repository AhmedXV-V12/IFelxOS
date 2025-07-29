#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stddef.h>
#include <stdbool.h>

void keyboard_readline(char* buf, size_t maxlen);
bool keyboard_check_esc(void);

#endif
