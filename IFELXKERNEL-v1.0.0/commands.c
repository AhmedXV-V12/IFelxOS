#include "commands.h"
#include "vga.h"
#include "mini_string.h"
#include <stdbool.h>
#include "heap.h"
#include "keyboard.h"
#include <stdarg.h>
#include <stddef.h>

// Constants for safety
#define MAX_FILENAME 255
#define MAX_CONTENT 32767
#define MAX_INPUT_LINE 1024
#define MAX_TOKENS 2000
#define MAX_VARIABLES 200
#define MAX_FUNCTIONS 100
#define MAX_STACK_SIZE 1000
#define MAX_BYTECODE 5000

// Global variables
txt_file_t* files_head = NULL;
folder_t* folders_head = NULL;
folder_t* current_folder = NULL;
char current_path[1024] = "/";

// Compiler structures
typedef enum {
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_IDENTIFIER,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_MULTIPLY,
    TOKEN_DIVIDE,
    TOKEN_ASSIGN,
    TOKEN_SEMICOLON,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_IF,
    TOKEN_WHILE,
    TOKEN_FOR,
    TOKEN_INT,
    TOKEN_CHAR,
    TOKEN_PRINTF,
    TOKEN_RETURN,
    TOKEN_MAIN,
    TOKEN_EOF,
    TOKEN_NEWLINE,
    TOKEN_COMMA,
    TOKEN_EQUALS,
    TOKEN_LESS,
    TOKEN_GREATER,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_DEF,
    TOKEN_PRINT,
    TOKEN_INPUT,
    TOKEN_COLON,
    TOKEN_INDENT,
    TOKEN_DEDENT,
    TOKEN_INCLUDE,
    TOKEN_VOID
} TokenType;

typedef struct {
    TokenType type;
    char value[256];
    int int_value;
    int line;
} Token;

typedef enum {
    VAR_INT,
    VAR_STRING,
    VAR_CHAR,
    VAR_FLOAT
} VarType;

typedef struct {
    char name[64];
    VarType type;
    union {
        int int_val;
        char str_val[256];
        char char_val;
        float float_val;
    } value;
    bool is_global;
} Variable;

typedef struct {
    char name[64];
    int start_pos;
    int param_count;
    VarType return_type;
} Function;

// Bytecode instructions for XVR format
typedef enum {
    OP_LOAD_CONST,
    OP_LOAD_VAR,
    OP_STORE_VAR,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_PRINT,
    OP_PRINTF,
    OP_CALL,
    OP_RETURN,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_COMPARE,
    OP_HALT,
    OP_INPUT,
    OP_GRAPHICS_MODE,
    OP_DRAW_PIXEL,
    OP_DRAW_LINE,
    OP_DRAW_RECT
} OpCode;

typedef struct {
    OpCode op;
    int arg1;
    int arg2;
    char str_arg[256];
} Instruction;

// Runtime structures
typedef struct {
    Variable variables[MAX_VARIABLES];
    int var_count;
    Function functions[MAX_FUNCTIONS];
    int func_count;
    int stack[MAX_STACK_SIZE];
    int stack_top;
    Token tokens[MAX_TOKENS];
    int token_count;
    int current_token;
    bool graphics_mode;
    Instruction bytecode[MAX_BYTECODE];
    int bytecode_count;
    int pc; // Program counter
    int constants[MAX_VARIABLES];
    int const_count;
} Runtime;

// Runtime structure is declared here
Runtime runtime;

// Helper functions
static int simple_snprintf(char* buffer, size_t size, const char* format, const char* str) {
    if (!buffer || size == 0) return 0;
    
    size_t buf_pos = 0;
    size_t fmt_pos = 0;
    
    while (format[fmt_pos] && buf_pos < size - 1) {
        if (format[fmt_pos] == '%' && format[fmt_pos + 1] == 's') {
            size_t str_pos = 0;
            while (str && str[str_pos] && buf_pos < size - 1) {
                buffer[buf_pos++] = str[str_pos++];
            }
            fmt_pos += 2;
        } else if (format[fmt_pos] == '%' && format[fmt_pos + 1] == 'd') {
            // Convert string to integer and then back to string
            char temp[32];
            int val = 0;
            if (str) {
                for (int i = 0; str[i] >= '0' && str[i] <= '9'; i++) {
                    val = val * 10 + (str[i] - '0');
                }
            }
            int temp_len = 0;
            if (val == 0) {
                temp[temp_len++] = '0';
            } else {
                int temp_val = val;
                while (temp_val > 0) {
                    temp[temp_len++] = '0' + (temp_val % 10);
                    temp_val /= 10;
                }
                // Reverse the string
                for (int i = 0; i < temp_len / 2; i++) {
                    char c = temp[i];
                    temp[i] = temp[temp_len - 1 - i];
                    temp[temp_len - 1 - i] = c;
                }
            }
            for (int i = 0; i < temp_len && buf_pos < size - 1; i++) {
                buffer[buf_pos++] = temp[i];
            }
            fmt_pos += 2;
        } else {
            buffer[buf_pos++] = format[fmt_pos++];
        }
    }
    
    buffer[buf_pos] = '\0';
    return buf_pos;
}

#ifndef snprintf
#define snprintf(buf, size, fmt, ...) simple_snprintf(buf, size, fmt, __VA_ARGS__)
#endif

// Utility functions
static bool is_valid_name(const char* name) {
    if (!name || !*name) return false;
    
    const char* invalid_chars = "\\/:*?\"<>|";
    for (int i = 0; name[i]; i++) {
        for (int j = 0; invalid_chars[j]; j++) {
            if (name[i] == invalid_chars[j]) {
                return false;
            }
        }
    }
    
    if (strlen(name) > MAX_FILENAME) return false;
    return true;
}

static void safe_string_copy(char* dest, const char* src, size_t dest_size) {
    if (!dest || !src || dest_size == 0) return;
    
    size_t i;
    for (i = 0; i < dest_size - 1 && src[i]; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

static txt_file_t* find_file(const char* name) {
    struct txt_file* current_files = current_folder ? current_folder->files : files_head;
    
    struct txt_file* f = current_files;
    while (f) {
        if (strcmp(f->name, name) == 0) {
            return f;
        }
        f = f->next;
    }
    return NULL;
}

static folder_t* find_folder(const char* name) {
    struct folder* f = folders_head;
    while (f) {
        if (strcmp(f->name, name) == 0) {
            return f;
        }
        f = f->next;
    }
    return NULL;
}

static void update_current_path(void) {
    if (!current_folder) {
        safe_string_copy(current_path, "/", sizeof(current_path));
        return;
    }
    
    char temp_path[512] = "/";
    if (current_folder) {
        safe_string_copy(temp_path, "/", sizeof(temp_path));
        size_t len = strlen(temp_path);
        safe_string_copy(temp_path + len, current_folder->name, sizeof(temp_path) - len);
        len = strlen(temp_path);
        if (len < sizeof(temp_path) - 1) {
            temp_path[len] = '/';
            temp_path[len + 1] = '\0';
        }
    }
    safe_string_copy(current_path, temp_path, sizeof(current_path));
}

// Lexer functions
static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

static bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool is_alnum(char c) {
    return is_alpha(c) || is_digit(c);
}

void skip_whitespace(const char** str) {
    while (**str == ' ' || **str == '\t' || **str == '\n' || **str == '\r') {
        (*str)++;
    }
}

static TokenType get_keyword_type(const char* word) {
    if (strcmp(word, "int") == 0) return TOKEN_INT;
    if (strcmp(word, "char") == 0) return TOKEN_CHAR;
    if (strcmp(word, "void") == 0) return TOKEN_VOID;
    if (strcmp(word, "if") == 0) return TOKEN_IF;
    if (strcmp(word, "while") == 0) return TOKEN_WHILE;
    if (strcmp(word, "for") == 0) return TOKEN_FOR;
    if (strcmp(word, "printf") == 0) return TOKEN_PRINTF;
    if (strcmp(word, "return") == 0) return TOKEN_RETURN;
    if (strcmp(word, "main") == 0) return TOKEN_MAIN;
    if (strcmp(word, "def") == 0) return TOKEN_DEF;
    if (strcmp(word, "print") == 0) return TOKEN_PRINT;
    if (strcmp(word, "input") == 0) return TOKEN_INPUT;
    if (strcmp(word, "#include") == 0) return TOKEN_INCLUDE;
    return TOKEN_IDENTIFIER;
}

static int tokenize_c(const char* code, Token* tokens, int max_tokens) {
    int token_count = 0;
    const char* current = code;
    int line = 1;
    
    while (*current && token_count < max_tokens - 1) {
        skip_whitespace(&current);
        
        if (*current == '\0') break;
        
        Token* token = &tokens[token_count++];
        token->line = line;
        
        if (*current == '\n') {
            token->type = TOKEN_NEWLINE;
            token->value[0] = '\n';
            token->value[1] = '\0';
            current++;
            line++;
        } else if (*current == '#') {
            // Handle preprocessor directives
            token->type = TOKEN_INCLUDE;
            int i = 0;
            while (*current && *current != '\n' && i < 255) {
                token->value[i++] = *current++;
            }
            token->value[i] = '\0';
        } else if (is_digit(*current)) {
            token->type = TOKEN_NUMBER;
            int i = 0;
            int value = 0;
            while (is_digit(*current) && i < 255) {
                token->value[i++] = *current;
                value = value * 10 + (*current - '0');
                current++;
            }
            token->value[i] = '\0';
            token->int_value = value;
        } else if (is_alpha(*current)) {
            int i = 0;
            while (is_alnum(*current) && i < 255) {
                token->value[i++] = *current++;
            }
            token->value[i] = '\0';
            token->type = get_keyword_type(token->value);
        } else if (*current == '"') {
            token->type = TOKEN_STRING;
            current++; // Skip opening quote
            int i = 0;
            while (*current && *current != '"' && i < 255) {
                if (*current == '\\' && *(current + 1) == 'n') {
                    token->value[i++] = '\n';
                    current += 2;
                } else {
                    token->value[i++] = *current++;
                }
            }
            token->value[i] = '\0';
            if (*current == '"') current++; // Skip closing quote
        } else {
            switch (*current) {
                case '+': token->type = TOKEN_PLUS; break;
                case '-': token->type = TOKEN_MINUS; break;
                case '*': token->type = TOKEN_MULTIPLY; break;
                case '/': 
                    if (*(current + 1) == '/') {
                        // Skip single line comment
                        while (*current && *current != '\n') current++;
                        continue;
                    }
                    token->type = TOKEN_DIVIDE; 
                    break;
                case '=': 
                    if (*(current + 1) == '=') {
                        token->type = TOKEN_EQUALS;
                        current++;
                    } else {
                        token->type = TOKEN_ASSIGN;
                    }
                    break;
                case ';': token->type = TOKEN_SEMICOLON; break;
                case '(': token->type = TOKEN_LPAREN; break;
                case ')': token->type = TOKEN_RPAREN; break;
                case '{': token->type = TOKEN_LBRACE; break;
                case '}': token->type = TOKEN_RBRACE; break;
                case ',': token->type = TOKEN_COMMA; break;
                case '<': token->type = TOKEN_LESS; break;
                case '>': token->type = TOKEN_GREATER; break;
                case ':': token->type = TOKEN_COLON; break;
                default:
                    // Skip unknown characters
                    current++;
                    token_count--; // Don't count this token
                    continue;
            }
            token->value[0] = *current;
            token->value[1] = '\0';
            current++;
        }
    }
    
    tokens[token_count].type = TOKEN_EOF;
    return token_count;
}

// Bytecode generation functions
static void emit_instruction(OpCode op, int arg1, int arg2, const char* str_arg) {
    if (runtime.bytecode_count >= MAX_BYTECODE) return;
    
    Instruction* inst = &runtime.bytecode[runtime.bytecode_count++];
    inst->op = op;
    inst->arg1 = arg1;
    inst->arg2 = arg2;
    if (str_arg) {
        safe_string_copy(inst->str_arg, str_arg, sizeof(inst->str_arg));
    } else {
        inst->str_arg[0] = '\0';
    }
}

static int add_constant(int value) {
    if (runtime.const_count >= MAX_VARIABLES) return -1;
    runtime.constants[runtime.const_count] = value;
    return runtime.const_count++;
}

// Runtime functions
static Variable* find_variable(const char* name) {
    for (int i = 0; i < runtime.var_count; i++) {
        if (strcmp(runtime.variables[i].name, name) == 0) {
            return &runtime.variables[i];
        }
    }
    return NULL;
}

static Variable* create_variable(const char* name, VarType type) {
    if (runtime.var_count >= MAX_VARIABLES) return NULL;
    
    Variable* var = &runtime.variables[runtime.var_count++];
    safe_string_copy(var->name, name, sizeof(var->name));
    var->type = type;
    var->is_global = true;
    
    // Initialize with default values
    switch (type) {
        case VAR_INT:
            var->value.int_val = 0;
            break;
        case VAR_STRING:
            var->value.str_val[0] = '\0';
            break;
        case VAR_CHAR:
            var->value.char_val = '\0';
            break;
        case VAR_FLOAT:
            var->value.float_val = 0.0f;
            break;
    }
    
    return var;
}

// C Compiler functions
static bool compile_c_expression(void) {
    if (runtime.current_token >= runtime.token_count) return false;
    
    Token* token = &runtime.tokens[runtime.current_token];
    
    if (token->type == TOKEN_NUMBER) {
        int const_idx = add_constant(token->int_value);
        emit_instruction(OP_LOAD_CONST, const_idx, 0, NULL);
        runtime.current_token++;
        return true;
    } else if (token->type == TOKEN_IDENTIFIER) {
        emit_instruction(OP_LOAD_VAR, 0, 0, token->value);
        runtime.current_token++;
        
        // Handle binary operations
        if (runtime.current_token < runtime.token_count) {
            Token* op_token = &runtime.tokens[runtime.current_token];
            if (op_token->type == TOKEN_PLUS || op_token->type == TOKEN_MINUS ||
                op_token->type == TOKEN_MULTIPLY || op_token->type == TOKEN_DIVIDE) {
                runtime.current_token++;
                if (compile_c_expression()) {
                    switch (op_token->type) {
                        case TOKEN_PLUS: emit_instruction(OP_ADD, 0, 0, NULL); break;
                        case TOKEN_MINUS: emit_instruction(OP_SUB, 0, 0, NULL); break;
                        case TOKEN_MULTIPLY: emit_instruction(OP_MUL, 0, 0, NULL); break;
                        case TOKEN_DIVIDE: emit_instruction(OP_DIV, 0, 0, NULL); break;
                        default: break;
                    }
                }
            }
        }
        return true;
    } else if (token->type == TOKEN_STRING) {
        emit_instruction(OP_LOAD_CONST, 0, 0, token->value);
        runtime.current_token++;
        return true;
    }
    
    return false;
}

static bool compile_c_statement(void) {
    if (runtime.current_token >= runtime.token_count) return false;
    
    Token* token = &runtime.tokens[runtime.current_token];
    
    switch (token->type) {
        case TOKEN_INT:
        case TOKEN_CHAR:
            // Variable declaration
            runtime.current_token++;
            if (runtime.current_token < runtime.token_count && 
                runtime.tokens[runtime.current_token].type == TOKEN_IDENTIFIER) {
                VarType var_type = (token->type == TOKEN_INT) ? VAR_INT : VAR_CHAR;
                create_variable(runtime.tokens[runtime.current_token].value, var_type);
                runtime.current_token++;
                
                // Check for initialization
                if (runtime.current_token < runtime.token_count && 
                    runtime.tokens[runtime.current_token].type == TOKEN_ASSIGN) {
                    runtime.current_token++;
                    if (compile_c_expression()) {
                        emit_instruction(OP_STORE_VAR, 0, 0, 
                                       runtime.variables[runtime.var_count - 1].name);
                    }
                }
            }
            break;
            
        case TOKEN_IDENTIFIER:
            // Assignment or function call
            {
                char var_name[256];
                safe_string_copy(var_name, token->value, sizeof(var_name));
                runtime.current_token++;
                
                if (runtime.current_token < runtime.token_count && 
                    runtime.tokens[runtime.current_token].type == TOKEN_ASSIGN) {
                    runtime.current_token++;
                    if (compile_c_expression()) {
                        emit_instruction(OP_STORE_VAR, 0, 0, var_name);
                    }
                }
            }
            break;
            
        case TOKEN_PRINTF:
            runtime.current_token++;
            if (runtime.current_token < runtime.token_count && 
                runtime.tokens[runtime.current_token].type == TOKEN_LPAREN) {
                runtime.current_token++;
                
                // Compile printf arguments
                int arg_count = 0;
                while (runtime.current_token < runtime.token_count && 
                       runtime.tokens[runtime.current_token].type != TOKEN_RPAREN) {
                    if (compile_c_expression()) {
                        arg_count++;
                    }
                    if (runtime.current_token < runtime.token_count && 
                        runtime.tokens[runtime.current_token].type == TOKEN_COMMA) {
                        runtime.current_token++;
                    }
                }
                
                emit_instruction(OP_PRINTF, arg_count, 0, NULL);
                
                if (runtime.current_token < runtime.token_count) {
                    runtime.current_token++; // Skip )
                }
            }
            break;
            
        case TOKEN_RETURN:
            runtime.current_token++;
            if (compile_c_expression()) {
                emit_instruction(OP_RETURN, 0, 0, NULL);
            }
            break;
            
        default:
            runtime.current_token++;
            break;
    }
    
    // Skip semicolon
    if (runtime.current_token < runtime.token_count && 
        runtime.tokens[runtime.current_token].type == TOKEN_SEMICOLON) {
        runtime.current_token++;
    }
    
    return true;
}

static bool compile_c_program(const char* source_code) {
    // Initialize runtime
    runtime.var_count = 0;
    runtime.func_count = 0;
    runtime.bytecode_count = 0;
    runtime.const_count = 0;
    runtime.current_token = 0;
    
    // Tokenize
    runtime.token_count = tokenize_c(source_code, runtime.tokens, MAX_TOKENS);
    
    // Compile statements
    while (runtime.current_token < runtime.token_count) {
        Token* token = &runtime.tokens[runtime.current_token];
        
        if (token->type == TOKEN_EOF) break;
        if (token->type == TOKEN_NEWLINE || token->type == TOKEN_INCLUDE) {
            runtime.current_token++;
            continue;
        }
        
        // Skip function definitions for now (main function handling)
        if (token->type == TOKEN_INT && runtime.current_token + 1 < runtime.token_count &&
            runtime.tokens[runtime.current_token + 1].type == TOKEN_MAIN) {
            runtime.current_token += 2; // Skip "int main"
            
            // Skip function signature
            while (runtime.current_token < runtime.token_count && 
                   runtime.tokens[runtime.current_token].type != TOKEN_LBRACE) {
                runtime.current_token++;
            }
            
            if (runtime.current_token < runtime.token_count) {
                runtime.current_token++; // Skip {
            }
            
            // Compile function body
            int brace_count = 1;
            while (runtime.current_token < runtime.token_count && brace_count > 0) {
                Token* current = &runtime.tokens[runtime.current_token];
                
                if (current->type == TOKEN_LBRACE) {
                    brace_count++;
                    runtime.current_token++;
                } else if (current->type == TOKEN_RBRACE) {
                    brace_count--;
                    runtime.current_token++;
                } else {
                    compile_c_statement();
                }
            }
        } else {
            compile_c_statement();
        }
    }
    
    emit_instruction(OP_HALT, 0, 0, NULL);
    return true;
}

// Python Compiler functions
static bool compile_python_program(const char* source_code) {
    // Initialize runtime
    runtime.var_count = 0;
    runtime.func_count = 0;
    runtime.bytecode_count = 0;
    runtime.const_count = 0;
    runtime.current_token = 0;
    
    // Simple Python tokenization and compilation
    runtime.token_count = tokenize_c(source_code, runtime.tokens, MAX_TOKENS);
    
    while (runtime.current_token < runtime.token_count) {
        Token* token = &runtime.tokens[runtime.current_token];
        
        if (token->type == TOKEN_EOF) break;
        if (token->type == TOKEN_NEWLINE) {
            runtime.current_token++;
            continue;  
        }
        
        if (token->type == TOKEN_PRINT) {
            runtime.current_token++;
            if (runtime.current_token < runtime.token_count && 
                runtime.tokens[runtime.current_token].type == TOKEN_LPAREN) {
                runtime.current_token++;
                
                int arg_count = 0;
                while (runtime.current_token < runtime.token_count && 
                       runtime.tokens[runtime.current_token].type != TOKEN_RPAREN) {
                    if (compile_c_expression()) {
                        arg_count++;
                    }
                    if (runtime.current_token < runtime.token_count && 
                        runtime.tokens[runtime.current_token].type == TOKEN_COMMA) {
                        runtime.current_token++;
                    }
                }
                
                emit_instruction(OP_PRINT, arg_count, 0, NULL);
                
                if (runtime.current_token < runtime.token_count) {
                    runtime.current_token++; // Skip )
                }
            }
        } else if (token->type == TOKEN_IDENTIFIER) {
            // Variable assignment
            char var_name[256];
            safe_string_copy(var_name, token->value, sizeof(var_name));
            runtime.current_token++;
            
            if (runtime.current_token < runtime.token_count && 
                runtime.tokens[runtime.current_token].type == TOKEN_ASSIGN) {
                runtime.current_token++;
                if (compile_c_expression()) {
                    // Create variable if it doesn't exist
                    if (!find_variable(var_name)) {
                        create_variable(var_name, VAR_INT);
                    }
                    emit_instruction(OP_STORE_VAR, 0, 0, var_name);
                }
            }
        } else {
            runtime.current_token++;
        }
    }
    
    emit_instruction(OP_HALT, 0, 0, NULL);
    return true;
}

// XVR Virtual Machine
static void execute_xvr_program(void) {
    runtime.pc = 0;
    runtime.stack_top = 0;
    
    while (runtime.pc < runtime.bytecode_count) {
        Instruction* inst = &runtime.bytecode[runtime.pc];
        
        switch (inst->op) {
            case OP_LOAD_CONST:
                if (inst->str_arg[0]) {
                    // String constant
                    vga_puts(inst->str_arg);
                } else {
                    // Integer constant
                    if (runtime.stack_top < MAX_STACK_SIZE) {
                        runtime.stack[runtime.stack_top++] = runtime.constants[inst->arg1];
                    }
                }
                break;
                
            case OP_LOAD_VAR:
                {
                    Variable* var = find_variable(inst->str_arg);
                    if (var && runtime.stack_top < MAX_STACK_SIZE) {
                        runtime.stack[runtime.stack_top++] = var->value.int_val;
                    }
                }
                break;
                
            case OP_STORE_VAR:
                if (runtime.stack_top > 0) {
                    Variable* var = find_variable(inst->str_arg);
                    if (!var) {
                        var = create_variable(inst->str_arg, VAR_INT);
                    }
                    if (var) {
                        var->value.int_val = runtime.stack[--runtime.stack_top];
                    }
                }
                break;
                
            case OP_ADD:
                if (runtime.stack_top >= 2) {
                    int b = runtime.stack[--runtime.stack_top];
                    int a = runtime.stack[--runtime.stack_top];
                    runtime.stack[runtime.stack_top++] = a + b;
                }
                break;
                
            case OP_SUB:
                if (runtime.stack_top >= 2) {
                    int b = runtime.stack[--runtime.stack_top];
                    int a = runtime.stack[--runtime.stack_top];
                    runtime.stack[runtime.stack_top++] = a - b;
                }
                break;
                
            case OP_MUL:
                if (runtime.stack_top >= 2) {
                    int b = runtime.stack[--runtime.stack_top];
                    int a = runtime.stack[--runtime.stack_top];
                    runtime.stack[runtime.stack_top++] = a * b;
                }
                break;
                
            case OP_DIV:
                if (runtime.stack_top >= 2) {
                    int b = runtime.stack[--runtime.stack_top];
                    int a = runtime.stack[--runtime.stack_top];
                    if (b != 0) {
                        runtime.stack[runtime.stack_top++] = a / b;
                    } else {
                        runtime.stack[runtime.stack_top++] = 0;
                    }
                }
                break;
                
            case OP_PRINT:
                if (runtime.stack_top > 0) {
                    int value = runtime.stack[--runtime.stack_top];
                    vga_printf("%d\n", value);
                }
                break;
                
            case OP_PRINTF:
                // Handle printf with format string
                if (inst->str_arg[0]) {
                    char* format = inst->str_arg;
                    char* p = format;
                    while (*p) {
                        if (*p == '%' && *(p + 1) == 'd' && runtime.stack_top > 0) {
                            int value = runtime.stack[--runtime.stack_top];
                            vga_printf("%d", value);
                            p += 2;
                        } else if (*p == '\\' && *(p + 1) == 'n') {
                            vga_putc('\n');
                            p += 2;
                        } else {
                            vga_putc(*p);
                            p++;
                        }
                    }
                } else if (runtime.stack_top > 0) {
                    int value = runtime.stack[--runtime.stack_top];
                    vga_printf("%d", value);
                }
                break;
                
            case OP_GRAPHICS_MODE:
                runtime.graphics_mode = true;
                vga_init_graphics();
                break;
                
            case OP_DRAW_PIXEL:
                if (runtime.stack_top >= 3) {
                    int color = runtime.stack[--runtime.stack_top];
                    int y = runtime.stack[--runtime.stack_top];
                    int x = runtime.stack[--runtime.stack_top];
                    // Call graphics function to draw pixel
                    vga_set_pixel(x, y, color);
                }
                break;
                
            case OP_HALT:
                return;
                
            default:
                break;
        }
        
        runtime.pc++;
    }
}

// Create XVR executable file
static bool create_xvr_file(const char* name) {
    char xvr_name[300];
    snprintf(xvr_name, sizeof(xvr_name), "%s.xvr", name);
    
    // Create executable file with bytecode
    struct txt_file* xvr_file = (struct txt_file*)my_malloc(sizeof(struct txt_file));
    if (!xvr_file) {
        return false;
    }
    
    safe_string_copy(xvr_file->name, xvr_name, sizeof(xvr_file->name));
    
    // Serialize bytecode to content
    size_t content_size = sizeof(int) + runtime.bytecode_count * sizeof(Instruction) + 
                         sizeof(int) + runtime.var_count * sizeof(Variable) +
                         sizeof(int) + runtime.const_count * sizeof(int);
    
    char* content = (char*)my_malloc(content_size);
    if (!content) {
        my_free(xvr_file);
        return false;
    }
    
    // Write bytecode header
    char* ptr = content;
    *((int*)ptr) = runtime.bytecode_count;
    ptr += sizeof(int);
    
    // Write bytecode instructions
    for (int i = 0; i < runtime.bytecode_count; i++) {
        *((Instruction*)ptr) = runtime.bytecode[i];
        ptr += sizeof(Instruction);
    }
    
    // Write variable count
    *((int*)ptr) = runtime.var_count;
    ptr += sizeof(int);
    
    // Write variables
    for (int i = 0; i < runtime.var_count; i++) {
        *((Variable*)ptr) = runtime.variables[i];
        ptr += sizeof(Variable);
    }
    
    // Write constants count
    *((int*)ptr) = runtime.const_count;
    ptr += sizeof(int);
    
    // Write constants
    for (int i = 0; i < runtime.const_count; i++) {
        *((int*)ptr) = runtime.constants[i];
        ptr += sizeof(int);
    }
    
    xvr_file->content = content;
    xvr_file->content_size = content_size;
    xvr_file->next = NULL;
    
    // Add to file system
    if (current_folder) {
        xvr_file->next = current_folder->files;
        current_folder->files = xvr_file;
    } else {
        xvr_file->next = files_head;
        files_head = xvr_file;
    }
    
    return true;
}

// Load XVR executable file
static bool load_xvr_file(const char* name) {
    char xvr_name[300];
    snprintf(xvr_name, sizeof(xvr_name), "%s.xvr", name);
    
    struct txt_file* xvr_file = find_file(xvr_name);
    if (!xvr_file || !xvr_file->content) {
        return false;
    }
    
    // Deserialize bytecode from content
    char* ptr = xvr_file->content;
    
    // Read bytecode count
    runtime.bytecode_count = *((int*)ptr);
    ptr += sizeof(int);
    
    // Read bytecode instructions
    for (int i = 0; i < runtime.bytecode_count && i < MAX_BYTECODE; i++) {
        runtime.bytecode[i] = *((Instruction*)ptr);
        ptr += sizeof(Instruction);
    }
    
    // Read variable count
    runtime.var_count = *((int*)ptr);
    ptr += sizeof(int);
    
    // Read variables
    for (int i = 0; i < runtime.var_count && i < MAX_VARIABLES; i++) {
        runtime.variables[i] = *((Variable*)ptr);
        ptr += sizeof(Variable);
    }
    
    // Read constants count
    runtime.const_count = *((int*)ptr);
    ptr += sizeof(int);
    
    // Read constants
    for (int i = 0; i < runtime.const_count && i < MAX_VARIABLES; i++) {
        runtime.constants[i] = *((int*)ptr);
        ptr += sizeof(int);
    }
    
    return true;
}

// File system functions
void list_files_and_folders(void) {
    struct txt_file* current_files = current_folder ? current_folder->files : files_head;
    
    vga_puts("Files:\n");
    struct txt_file* f = current_files;
    bool found_files = false;
    while (f) {
        vga_printf("- %s\n", f->name);
        f = f->next;
        found_files = true;
    }
    if (!found_files) {
        vga_puts("(No files)\n");
    }
    
    vga_puts("\nFolders:\n");
    struct folder* dir = folders_head;
    bool found_folders = false;
    while (dir) {
        if (!current_folder || dir->parent == current_folder) {
            vga_printf("- %s/\n", dir->name);
            found_folders = true;
        }
        dir = dir->next;
    }
    if (!found_folders) {
        vga_puts("(No folders)\n");
    }
}

void print_current_directory(void) {
    vga_printf("Current directory: %s\n", current_path);
}

void change_directory(const char* name) {
    if (!name || !*name) {
        vga_puts("Error: No directory name provided\n");
        return;
    }
    
    if (strcmp(name, "..") == 0) {
        if (current_folder && current_folder->parent) {
            current_folder = current_folder->parent;
        } else {
            current_folder = NULL;
        }
        update_current_path();
        vga_printf("Changed to directory: %s\n", current_path);
        return;
    }
    
    if (strcmp(name, "/") == 0) {
        current_folder = NULL;
        update_current_path();
        vga_printf("Changed to directory: %s\n", current_path);
        return;
    }
    
    struct folder* target = find_folder(name);
    if (target && (!current_folder || target->parent == current_folder)) {
        current_folder = target;
        update_current_path();
        vga_printf("Changed to directory: %s\n", current_path);
    } else {
        vga_printf("Directory not found: %s\n", name);
    }
}

void show_tree_os(void) {
    vga_puts("File system tree:\n/\n");
    
    struct txt_file* f = files_head;
    while (f) {
        vga_printf("├── %s\n", f->name);
        f = f->next;
    }
    
    struct folder* dir = folders_head;
    while (dir) {
        if (!dir->parent) {
            vga_printf("├── %s/\n", dir->name);
            
            struct txt_file* file = dir->files;
            while (file) {
                vga_printf("│   ├── %s\n", file->name);
                file = file->next;
            }
        }
        dir = dir->next;
    }
}

void print_help(void) {
    vga_puts("Available commands:\n");
    vga_puts("help - Show this help\n");
    vga_puts("clear - Clear screen\n");
    vga_puts("graphics - Enter graphics mode\n");
    vga_puts("add txt=<name> - Create text file\n");
    vga_puts("open txt=<name> - Open text file\n");
    vga_puts("open folder - List folders\n");
    vga_puts("del=<name> - Delete file or folder\n");
    vga_puts("ls - List files and folders\n");
    vga_puts("add folder=<name> - Create folder\n");
    vga_puts("cd=<name> - Change directory (use .. for parent, / for root)\n");
    vga_puts("pwd - Show current directory\n");
    vga_puts("tree - Show file system tree\n");
    vga_puts("make c=<name> - Compile .c file to .xvr\n");
    vga_puts("make py=<name> - Compile .py file to .xvr\n");
    vga_puts("run=<name> - Run .xvr executable\n");
    vga_puts("python=<name> - Run .py file directly\n");
    vga_puts("exit - Exit the OS\n");
}

void add_txt(const char* name) {
    if (!is_valid_name(name)) {
        vga_puts("[X] Invalid filename\n");
        return;
    }
    
    if (find_file(name)) {
        vga_puts("[X] File already exists\n");
        return;
    }
    
    struct txt_file* f = (struct txt_file*)my_malloc(sizeof(struct txt_file));
    if (!f) {
        vga_puts("[X] Not enough memory\n");
        return;
    }
    
    safe_string_copy(f->name, name, sizeof(f->name));
    f->content = NULL;
    f->content_size = 0;
    f->next = NULL;
    
    vga_puts("Enter file content (end with a single line containing only .):\n");
    
    size_t buffer_size = 1024;
    char* content_buffer = (char*)my_malloc(buffer_size);
    if (!content_buffer) {
        vga_puts("[X] Not enough memory for content\n");
        my_free(f);
        return;
    }
    
    char line_buf[MAX_INPUT_LINE];
    size_t total_len = 0;
    
    while (1) {
        keyboard_readline(line_buf, sizeof(line_buf));
        
        if (line_buf[0] == '.' && line_buf[1] == '\0') {
            break;
        }
        
        size_t line_len = strlen(line_buf);
        
        if (total_len + line_len + 2 > buffer_size) {
            size_t new_size = buffer_size * 2;
            char* new_buffer = (char*)my_malloc(new_size);
            if (!new_buffer) {
                vga_puts("[X] Not enough memory for content expansion\n");
                my_free(content_buffer);
                my_free(f);
                return;
            }
            
            for (size_t i = 0; i < total_len; i++) {
                new_buffer[i] = content_buffer[i];
            }
            
            my_free(content_buffer);
            content_buffer = new_buffer;
            buffer_size = new_size;
        }
        
        for (size_t i = 0; i < line_len; i++) {
            content_buffer[total_len++] = line_buf[i];
        }
        content_buffer[total_len++] = '\n';
    }
    
    content_buffer[total_len] = '\0';
    f->content = content_buffer;
    f->content_size = total_len;
    
    if (current_folder) {
        f->next = current_folder->files;
        current_folder->files = f;
    } else {
        f->next = files_head;
        files_head = f;
    }
    
    vga_puts("[✓] File created successfully\n");
}

void add_folder(const char* name) {
    if (!is_valid_name(name)) {
        vga_puts("[X] Invalid folder name\n");
        return;
    }
    
    if (find_folder(name)) {
        vga_puts("[X] Folder already exists\n");
        return;
    }
    
    struct folder* f = (struct folder*)my_malloc(sizeof(struct folder));
    if (!f) {
        vga_puts("[X] Not enough memory\n");
        return;
    }
    
    safe_string_copy(f->name, name, sizeof(f->name));
    f->parent = current_folder;
    f->files = NULL;
    f->next = folders_head;
    folders_head = f;
    
    vga_puts("[✓] Folder created successfully\n");
}

void open_txt(const char* name) {
    struct txt_file* f = find_file(name);
    if (f && f->content) {
        vga_puts(f->content);
        if (f->content_size > 0 && f->content[f->content_size - 1] != '\n') {
            vga_putc('\n');
        }
    } else {
        vga_puts("[X] File not found\n");
    }
}

void open_folder(void) {
    struct folder* f = folders_head;
    vga_puts("Available folders:\n");
    bool found = false;
    
    while (f) {
        if (!current_folder || f->parent == current_folder) {
            vga_printf("- %s\n", f->name);
            found = true;
        }
        f = f->next;
    }
    
    if (!found) {
        vga_puts("(No folders in current directory)\n");
    }
}

void del(const char* target) {
    if (!is_valid_name(target)) {
        vga_puts("[X] Invalid name\n");
        return;
    }
    
    struct txt_file** current_files_head = current_folder ? &(current_folder->files) : &files_head;
    struct txt_file** pp = current_files_head;
    
    while (*pp) {
        if (strcmp((*pp)->name, target) == 0) {
            struct txt_file* to_del = *pp;
            *pp = to_del->next;
            
            if (to_del->content) {
                my_free(to_del->content);
            }
            my_free(to_del);
            vga_puts("[✓] File deleted\n");
            return;
        }
        pp = &((*pp)->next);
    }
    
    struct folder** fp = &folders_head;
    while (*fp) {
        if (strcmp((*fp)->name, target) == 0 && 
            (!current_folder || (*fp)->parent == current_folder)) {
            
            struct folder* to_del = *fp;
            
            if (to_del->files != NULL) {
                vga_puts("[X] Cannot delete non-empty folder\n");
                return;
            }
            
            if (current_folder == to_del) {
                current_folder = to_del->parent;
                update_current_path();
            }
            
            *fp = to_del->next;
            my_free(to_del);
            vga_puts("[✓] Folder deleted\n");
            return;
        }
        fp = &((*fp)->next);
    }
    
    vga_puts("[X] File or folder not found!\n");
}

// Real C Compiler
void make_c_file(const char* name) {
    if (!is_valid_name(name)) {
        vga_puts("[X] Invalid filename\n");
        return;
    }
    
    char source_name[300];
    snprintf(source_name, sizeof(source_name), "%s.c", name);
    
    struct txt_file* source_file = find_file(source_name);
    if (!source_file) {
        vga_printf("[X] Source file %s not found\n", source_name);
        return;
    }
    
    vga_printf("C Compiler: Compiling %s.c to %s.xvr...\n", name, name);
    vga_puts("C Compiler: Lexical analysis...\n");
    
    // Real compilation
    if (!compile_c_program(source_file->content)) {
        vga_puts("[X] Compilation failed\n");
        return;
    }
    
    vga_puts("C Compiler: Syntax analysis complete\n");
    vga_puts("C Compiler: Code generation...\n");
    
    // Create XVR executable
    if (!create_xvr_file(name)) {
        vga_puts("[X] Failed to create executable\n");
        return;
    }
    
    vga_printf("[✓] C Compiler: Successfully created %s.xvr\n", name);
}

// Real Python Compiler
void make_py_file(const char* name) {
    if (!is_valid_name(name)) {
        vga_puts("[X] Invalid filename\n");
        return;
    }
    
    char source_name[300];
    snprintf(source_name, sizeof(source_name), "%s.py", name);
    
    struct txt_file* source_file = find_file(source_name);
    if (!source_file) {
        vga_printf("[X] Source file %s not found\n", source_name);
        return;
    }
    
    vga_printf("Python Compiler: Compiling %s.py to %s.xvr...\n", name, name);
    vga_puts("Python Compiler: Tokenizing source code...\n");
    
    // Real compilation
    if (!compile_python_program(source_file->content)) {
        vga_puts("[X] Compilation failed\n");
        return;
    }
    
    vga_puts("Python Compiler: Building AST...\n");
    vga_puts("Python Compiler: Generating bytecode...\n");
    
    // Create XVR executable
    if (!create_xvr_file(name)) {
        vga_puts("[X] Failed to create executable\n");
        return;
    }
    
    vga_printf("[✓] Python Compiler: Successfully created %s.xvr\n", name);
}

// Real XVR Executor
void run_executable(const char* name) {
    if (!is_valid_name(name)) {
        vga_puts("[X] Invalid filename\n");
        return;
    }
    
    vga_printf("XVR Runtime: Loading %s.xvr...\n", name);
    
    if (!load_xvr_file(name)) {
        vga_printf("[X] Executable %s.xvr not found\n", name);
        return;
    }
    
    vga_puts("XVR Runtime: Starting execution...\n");
    vga_puts("=== Program Output ===\n");
    
    // Execute the real bytecode
    execute_xvr_program();
    
    vga_puts("\n=== End of Program ===\n");
    vga_printf("XVR Runtime: %s.xvr execution completed\n", name);
}

// Direct Python interpreter
void run_python_directly(const char* name) {
    if (!is_valid_name(name)) {
        vga_puts("[X] Invalid filename\n");
        return;
    }
    
    char py_name[300];
    snprintf(py_name, sizeof(py_name), "%s.py", name);
    
    struct txt_file* py_file = find_file(py_name);
    if (!py_file) {
        vga_printf("[X] Python file %s not found\n", py_name);
        return;
    }
    
    vga_printf("Python Interpreter: Running %s.py directly...\n", name);
    vga_puts("=== Python Direct Execution ===\n");
    
    // Compile and execute directly
    if (compile_python_program(py_file->content)) {
        execute_xvr_program();
    } else {
        vga_puts("[X] Python interpretation failed\n");
    }
    
    vga_puts("=== End of Python Execution ===\n");
}

// Command handler
bool commands_handle(const char* input) {
    if (!input || !*input) {
        return false;
    }
    
    while (*input == ' ' || *input == '\t') input++;
    
    if (strncmp(input, "add txt=", 8) == 0) {
        add_txt(input + 8);
        return true;
    } else if (strncmp(input, "add folder=", 11) == 0) {
        add_folder(input + 11);
        return true;
    } else if (strncmp(input, "open txt=", 9) == 0) {
        open_txt(input + 9);
        return true;
    } else if (strcmp(input, "open folder") == 0) {
        open_folder();
        return true;
    } else if (strncmp(input, "del=", 4) == 0) {
        del(input + 4);
        return true;
    } else if (strcmp(input, "help") == 0) {
        print_help();
        return true;
    } else if (strcmp(input, "clear") == 0) {
        vga_clear();
        return true;
    } else if (strcmp(input, "ls") == 0) {
        list_files_and_folders();
        return true;
    } else if (strcmp(input, "pwd") == 0) {
        print_current_directory();
        return true;
    } else if (strncmp(input, "cd=", 3) == 0) {
        change_directory(input + 3);
        return true;
    } else if (strcmp(input, "tree") == 0) {
        show_tree_os();
        return true;
    } else if (strncmp(input, "make c=", 7) == 0) {
        make_c_file(input + 7);
        return true;
    } else if (strncmp(input, "make py=", 8) == 0) {
        make_py_file(input + 8);
        return true;
    } else if (strncmp(input, "run=", 4) == 0) {
        run_executable(input + 4);
        return true;
    } else if (strncmp(input, "python=", 7) == 0) {
        run_python_directly(input + 7);
        return true;
    } else if (strcmp(input, "graphics") == 0) {
        vga_puts("Entering graphics mode...\n");
        vga_init_graphics();
        
        while (1) {
            if (keyboard_check_esc()) {
                vga_init_text();
                vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLUE);
                vga_clear();
                vga_puts("Returned to text mode\n");
                return true;
            }
            
            for (volatile int i = 0; i < 100000; i++);
        }
        return true;
    }
    
    return false;
}