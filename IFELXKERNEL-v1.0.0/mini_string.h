#ifndef MINI_STRING_H
#define MINI_STRING_H
#include <stddef.h>
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, size_t n);
char* strncpy(char* dest, const char* src, size_t n);
size_t strlen(const char* s);
char* strcpy(char* dest, const char* src);
int vsnprintf(char* buf, size_t size, const char* fmt, __builtin_va_list args);
char* strstr(const char* haystack, const char* needle);
#endif
