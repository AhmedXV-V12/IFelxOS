#include "heap.h"

#define HEAP_SIZE (1024*1024)
static char heap[HEAP_SIZE];
static size_t heap_ptr = 0;

void* my_malloc(size_t size) {
    // Simple bump allocator - no free list management
    if (heap_ptr + size > HEAP_SIZE) return 0;
    void* ptr = &heap[heap_ptr];
    heap_ptr += size;
    return ptr;
}

void my_free(void* ptr) {
    // In this simple implementation, we don't actually free memory
    // A more complete implementation would manage a free list
    (void)ptr; // Prevent unused parameter warning
}
