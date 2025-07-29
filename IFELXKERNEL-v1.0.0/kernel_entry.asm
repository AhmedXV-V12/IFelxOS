section .multiboot
    align 4
    dd 0x1BADB002
    dd 0
    dd -(0x1BADB002)

section .text
    global start
start:
    mov esp, 0x90000
    extern kernel_main
    call kernel_main
    cli
.hang:
    hlt
    jmp .hang
