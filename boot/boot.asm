; ROCKET-OS stage 0 BIOS boot sector
BITS 16
ORG 0x7C00

PAYLOAD_SEGMENT equ 0x1000
PAYLOAD_SECTORS equ 128

start:
    cli
    xor ax, ax
    mov ds, ax
    mov ss, ax
    mov sp, 0x7C00
    sti
    mov [boot_drive], dl

    mov ax, 0x0003
    int 0x10
    mov ax, 0xB800
    mov es, ax
    xor di, di
    mov ax, 0x1F20              ; white on blue, blank cell
    mov cx, 80 * 25
    rep stosw

    mov di, 10 * 160 + 34 * 2
    mov si, title
.print_title:
    lodsb
    test al, al
    jz .load
    mov ah, 0x1F
    stosw
    jmp .print_title

.load:
    mov ax, PAYLOAD_SEGMENT
    mov es, ax
    xor bx, bx                  ; ES:BX = physical 0x10000
    mov word [current_sector], 2
    mov byte [current_head], 0
    mov word [current_cylinder], 0
    mov cx, PAYLOAD_SECTORS
.next_sector:
    push cx
    mov ah, 0x02
    mov al, 1
    mov ch, byte [current_cylinder]
    mov cl, byte [current_sector]
    mov dh, byte [current_head]
    mov dl, [boot_drive]
    int 0x13
    jc disk_error
    add bx, 512
    inc word [current_sector]
    cmp word [current_sector], 19
    jb .continue
    mov word [current_sector], 1
    inc byte [current_head]
    cmp byte [current_head], 2
    jb .continue
    mov byte [current_head], 0
    inc word [current_cylinder]
.continue:
    pop cx
    loop .next_sector
    jmp PAYLOAD_SEGMENT:0

disk_error:
    mov ax, 0xB800
    mov es, ax
    mov di, 12 * 160 + 30 * 2
    mov si, error_text
.print_error:
    lodsb
    test al, al
    jz .halt
    mov ah, 0x4F
    stosw
    jmp .print_error
.halt:
    cli
    hlt
    jmp .halt

boot_drive db 0
current_sector dw 2
current_head db 0
current_cylinder dw 0
title db 'ROCKET-OS', 0
error_text db 'BOOT ERROR: DISK READ FAILED', 0

times 510-($-$$) db 0
dw 0xAA55
