.section .text
.code32
.global _start
.extern kernel_main
_start:
    cli
    mov $0x90000, %esp
    mov %esp, %ebp
    call kernel_main
.hang:
    hlt
    jmp .hang
