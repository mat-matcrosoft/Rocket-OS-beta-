# ROCKET-OS

ROCKET-OS is a small DOS-like x86 operating-system experiment written in C and NASM assembly. The current alpha boots through BIOS in QEMU, displays a blue-and-white ROCKET-OS splash, enters 32-bit protected mode, and starts an interactive VGA text shell.

## Quick start

Read the complete build and FAT32 roadmap in docs/BUILD.md. The short path is:

    make image
    make run
    make test

Required tools for the current boot image are NASM, GNU Make, an i386 freestanding GCC/binutils toolchain, and QEMU. A host C compiler is also needed for the smoke tests. The default cross tools are i686-elf-gcc, i686-elf-ld, and i686-elf-objcopy; override them on the command line when your environment uses different names.

    make CC=gcc LD=ld OBJCOPY=objcopy image
    make HOST_CC=cc test

## Current boot image and FAT32 status

The current image is a fixed-size raw BIOS image. Stage 0 reads a bounded contiguous payload with BIOS CHS calls, Stage 1 enters protected mode, and the kernel starts the VGA shell. It is intentionally not yet a FAT32 filesystem image. The FAT32 data structures and cluster arithmetic are isolated in fs/ so the next Stage 2 loader can be added without changing the kernel shell ABI.

The planned FAT32 flow is documented in docs/BUILD.md and docs/DESIGN.md: read the BPB, locate the FAT and root directory, resolve 8.3/LFN names, follow cluster chains, load KERNEL.BIN, then transfer control to the protected-mode kernel.

## Shell

The alpha shell includes help, clear, about, echo, ls, cat, mkdir, rmdir, touch, rm, cp, mv, load, and run. Filesystem and RBE commands report their scaffold status until the FAT32 block device and user-mode loader milestones are completed.

## Layout

boot/ and stage/ contain the BIOS boot path; kernel/ contains the protected-mode kernel and VGA shell; fs/ contains FAT32 structures; rbe/ defines the RBE1 format; api/ contains the application ABI; apps/ contains example clients; tests/ contains host-side smoke tests; docs/ contains the architecture, API, and build roadmap.

## License

The repository keeps its existing BSD-3-Clause license.
