#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdbool.h>
#include <stddef.h>

// File system node types
typedef enum {
    TYPE_FILE,
    TYPE_FOLDER
} node_type_t;

// File system node structure
typedef struct fs_node {
    char name[256];
    node_type_t type;
    union {
        struct {
            char* content;
            size_t size;
        } file;
        struct {
            struct fs_node* children;
            struct fs_node* parent;
            struct fs_node* next;
        } folder;
    } data;
} fs_node_t;

// Global variables
extern fs_node_t* root_dir;
extern fs_node_t* current_dir;

typedef struct network {
    char name[64];
    bool is_wireless;
    int signal_strength;
    struct network* next;
} network_t;

extern network_t* networks_head;

typedef struct txt_file {
    char name[256];
    char* content;
    size_t content_size;
    struct txt_file* next;
} txt_file_t;

typedef struct folder {
    char name[256];
    struct folder* parent;
    struct folder* next;
    struct txt_file* files;
} folder_t;

extern txt_file_t* files_head;
extern folder_t* folders_head;

extern bool batch_mode;
extern int final_number;

// Basic I/O functions
void vga_putc(char c);
void vga_puts(const char* str);
void vga_printf(const char* format, ...);
void vga_putn(int n);
void vga_clear(void);
void vga_init_text(void);
void vga_setcolor(unsigned char fg, unsigned char bg);
void vga_set_graphics_mode(void);

// String functions (from mini_string.h)
size_t strlen(const char* str);
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, size_t n);
char* strncpy(char* dest, const char* src, size_t n);
char* strcpy(char* dest, const char* src);
int vsnprintf(char* buf, size_t size, const char* fmt, __builtin_va_list args);

// Memory management (from heap.h)
void* my_malloc(size_t size);
void my_free(void* ptr);
void* my_realloc(void* ptr, size_t size);

// File system functions
fs_node_t create_file(const char* name);
fs_node_t create_folder(const char* name);
void init_filesystem(void);
fs_node_t* find_node(const char* name);
bool add_node(fs_node_t* node);
void free_node(fs_node_t* node);
void ensure_filesystem(void);

// File system commands
void list_files_and_folders(void);
void print_current_directory(void);
void change_directory(const char* name);
void show_tree_os(void);
void print_help(void);
void add_txt(const char* name);
void add_folder(const char* name);
void open_txt(const char* name);
void open_folder(void);
void del(const char* target);
void print_tree_recursive(fs_node_t* node, int level);

// Command handlers
void make_xvr(const char* name);
void run_xvr(const char* name);
void handle_network_command(const char* args);

// Helper functions
bool starts_with(const char* str, const char* prefix);
bool str_equals(const char* a, const char* b);
void safe_strncpy(char* dest, const char* src, size_t max_len);
void skip_whitespace(const char** str);
char get_argument(const char* input, const char* prefix);

// Main command handler
bool commands_handle(const char* input);

// Keyboard functions (from keyboard.h)
void keyboard_readline(char* buf, size_t maxlen);
bool keyboard_check_esc(void);

#endif // COMMANDS_H
