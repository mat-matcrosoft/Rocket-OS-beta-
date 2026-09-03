# ROCKET-OS

ROCKET-OS is a small DOS-like x86 operating-system experiment written in C and NASM assembly. The current alpha boots through BIOS in QEMU, displays a blue-and-white ROCKET-OS splash, enters 32-bit protected mode, and starts an interactive VGA text shell.

## Build and run

Required tools: NASM, an i386 freestanding GCC/binutils toolchain, GNU Make, and QEMU.

    make image
    make run
    make test

If your environment exposes the cross compiler under another name, override the tools, for example: make CC=gcc LD=ld OBJCOPY=objcopy image, when 32-bit support is available.

## Shell

The alpha shell includes help, clear, about, echo, ls, cat, mkdir, rmdir, touch, rm, cp, mv, load, and run. Filesystem and RBE commands report their scaffold status until the FAT32 block device and user-mode loader milestones are completed.

## Layout

boot/ and stage/ contain the BIOS boot path; kernel/ contains the protected-mode kernel and VGA shell; fs/ contains FAT32 structures; rbe/ defines the RBE1 format; api/ contains the application ABI; apps/ contains example clients; tests/ contains host-side smoke tests; docs/ contains the architecture and API notes.

## License

The repository keeps its existing BSD-3-Clause license.
