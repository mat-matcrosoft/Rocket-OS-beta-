.section .rodata
.global rocket_stage2_header
rocket_stage2_header:
    .ascii "RKS2"
    .short 1
    .long 0x11000
    .long 0
