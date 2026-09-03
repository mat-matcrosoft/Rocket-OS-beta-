.section .text
.code32
.global syscall_entry
syscall_entry:
    ; Reserved entry point for the user/kernel syscall ABI.
    iret
