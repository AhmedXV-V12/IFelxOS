#include "mini_string.h"
#include <stddef.h>

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) { ++s1; ++s2; }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int strncmp(const char* s1, const char* s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) { ++s1; ++s2; --n; }
    if (n == 0) return 0;
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

char* strncpy(char* dest, const char* src, size_t n) {
    size_t i = 0;
    for (; i < n && src[i]; ++i) dest[i] = src[i];
    for (; i < n; ++i) dest[i] = '\0';
    return dest;
}

size_t strlen(const char* s) {
    size_t i = 0; while (s[i]) ++i; return i;
}

char* strcpy(char* dest, const char* src) {
    char* d = dest;
    while ((*d++ = *src++));
    return dest;
}

// Minimal vsnprintf: only supports %s and %d, no width/precision, no float
int vsnprintf(char* buf, size_t size, const char* fmt, __builtin_va_list args) {
    size_t i = 0;
    for (; *fmt && i + 1 < size; ++fmt) {
        if (*fmt == '%') {
            ++fmt;
            if (*fmt == 's') {
                const char* s = __builtin_va_arg(args, const char*);
                while (*s && i + 1 < size) buf[i++] = *s++;
            } else if (*fmt == 'd') {
                int v = __builtin_va_arg(args, int);
                char tmp[16];
                int n = 0, neg = 0;
                if (v < 0) { neg = 1; v = -v; }
                do { tmp[n++] = '0' + (v % 10); v /= 10; } while (v && n < 15);
                if (neg) tmp[n++] = '-';
                while (n && i + 1 < size) buf[i++] = tmp[--n];
            } else {
                buf[i++] = '%';
                if (i + 1 < size) buf[i++] = *fmt;
            }
        } else {
            buf[i++] = *fmt;
        }
    }
    buf[i] = 0;
    return i;
}

char* strstr(const char* haystack, const char* needle) {
    if (!*needle) return (char*)haystack;
    
    for (const char* p = haystack; *p; p++) {
        const char *h = p, *n = needle;
        while (*h && *n && (*h == *n)) {
            h++;
            n++;
        }
        if (!*n) return (char*)p;
    }
    return NULL;
}
