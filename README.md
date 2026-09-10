# ROCKET-OS

ROCKET-OS is a small DOS-like x86 operating-system experiment written in C and NASM assembly. The current alpha boots through BIOS in QEMU, displays a blue-and-white ROCKET-OS splash, enters 32-bit protected mode, and starts an interactive VGA text shell.

## Quick start

    make image
    make run
    make test

Required tools are NASM, GNU Make, an i386 freestanding GCC/binutils toolchain, QEMU, and a host C compiler for smoke tests. Override the cross tools when needed:

    make CC=gcc LD=ld OBJCOPY=objcopy image
    make HOST_CC=cc test

## Storage status

The kernel now includes a block-device manager, driver manager, ATA PIO support for the primary IDE master/slave with LBA28, and a FAT32 volume implementation. FAT32 validates the BPB, follows bounded cluster chains, lists directories, reads short 8.3 files, and replaces an existing file when its current cluster chain is large enough. The kernel mounts ata0 when a FAT32 superfloppy is present at LBA 0.

The BIOS image is still the original fixed-size development image: Stage 0 reads a bounded contiguous payload and Stage 1 enters protected mode. The next boot milestone is a FAT32 Stage 2 loader; the in-kernel storage stack is ready for that work.

See docs/STORAGE.md for the interfaces and current limitations.

## Shell

The shell includes help, clear, about, devices, ls, cat, and write. FAT32 path lookup currently uses short 8.3 names; LFN entries are ignored until checksum-validated LFN support is added.

## Layout

boot/ and stage/ contain the BIOS boot path; kernel/ contains the protected-mode kernel and VGA shell; drivers/ contains device and driver managers plus ATA PIO; fs/ contains the FAT32 volume and directory API; rbe/ defines the RBE1 format; api/ contains the application ABI; apps/ contains example clients; tests/ contains host-side smoke tests; docs/ contains architecture and storage documentation.

## License

The repository keeps its existing BSD-3-Clause license.

## Windows and v86

Для сборки в Windows через WSL2 и запуска ISO в браузерном v86 см. docs/WINDOWS.md.
