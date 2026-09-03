# ROCKET-OS build and FAT32 loading guide

## 1. Install the toolchain

The boot path needs NASM, GNU Make, QEMU, and a 32-bit freestanding compiler/binutils set:

- nasm
- i686-elf-gcc, i686-elf-ld, i686-elf-objcopy
- qemu-system-i386
- a host C compiler such as cc for smoke tests

If your system has a multilib GCC instead of an i686-elf cross compiler, pass the tools explicitly:

    make CC=gcc LD=ld OBJCOPY=objcopy image

The freestanding compiler must accept -m32, compile without a hosted runtime, and produce i386 ELF objects. Do not use the host compiler for the kernel unless it supports 32-bit code generation.

## 2. Build the current bootable alpha

From the repository root:

    make clean
    make image
    make run

The generated files are placed below build/. The image contains a 512-byte BIOS boot sector, a 4 KiB-padded Stage 1, and the linked kernel payload. Stage 0 reads the bounded payload with BIOS CHS reads, Stage 1 installs a flat GDT, and kernel_main starts the VGA shell.

Use the smoke tests separately:

    make test

The tests are host-side and do not require QEMU. They cover FAT32 cluster arithmetic and shell command-prefix parsing. They do not prove that the BIOS image boots; that check is performed with make run.

## 3. FAT32 image milestone

The current build image is not FAT32 yet. Do not copy files into build/rocket-os.img and expect ls or cat to see them. The next image pipeline should create a real partition or superfloppy image and populate it with KERNEL.BIN and .RBE applications.

A development image can be prepared with tools available on the host, for example:

    qemu-img create -f raw build/fat32.img 32M
    mkfs.fat -F 32 -n ROCKETOS build/fat32.img
    mmd -i build/fat32.img ::/SYSTEM
    mcopy -i build/fat32.img build/kernel/kernel.bin ::/SYSTEM/KERNEL.BIN

The exact mtools commands depend on the installed tool versions. The repository does not run them automatically yet because the Stage 2 loader still expects the fixed contiguous payload.

## 4. FAT32 Stage 2 implementation order

Implement the loader in this order so each step remains testable:

1. Add BIOS INT 13h extensions (AH=41h/42h) and a 64-bit LBA disk-read helper. Keep the CHS fallback for simple QEMU images.
2. Read sector 0 into a 512-byte BPB buffer and validate the FAT32 signature, 512-byte sectors, non-zero sectors-per-cluster, reserved-sector count, FAT count, and sectors-per-FAT.
3. Compute the layout: FAT start = reserved sectors; data start = reserved sectors + FAT count * sectors per FAT; cluster N LBA = data start + (N - 2) * sectors per cluster.
4. Read the FAT entry for the current cluster, mask it with 0x0FFFFFFF, and follow the chain until an end marker in the range 0x0FFFFFF8..0x0FFFFFFF. Reject bad and reserved cluster values.
5. Walk the root directory cluster chain. Support short 8.3 names first, then add LFN slots with checksum validation. Ignore deleted entries and stop at the 0x00 terminator.
6. Locate SYSTEM/KERNEL.BIN, load its clusters below the Stage 1 memory range, verify its size, and jump to the agreed kernel entry address.
7. Add a disk abstraction shared by the loader and kernel so filesystem code can be tested against a memory-backed sector device.

The loader must never trust a sector count or cluster number from disk without checking it against the image size and a maximum kernel size. A malformed FAT32 image should display a readable boot error instead of jumping to unchecked memory.

## 5. After FAT32 loading

After KERNEL.BIN loads correctly, implement the remaining milestones in this order:

- FAT32 read-only files: directory listing, open, read, stat, and cat.
- FAT32 writes: create, write, mkdir, delete, rename, copy, and cache flush.
- Shell path normalization and LFN-aware command arguments.
- RBE builder and loader: header, section bounds, checksum, CODE/DATA relocation policy, entry-point validation.
- Kernel heap and syscall dispatch; then GDT/TSS user mode before executing untrusted RBE code.
- Example applications compiled into RBE files and copied into the FAT32 image.
- QEMU regression scenarios for boot, malformed BPB, fragmented files, LFN names, shell I/O, and invalid RBE files.

## 6. Useful diagnostics

    make clean
    make image
    qemu-system-i386 -drive format=raw,file=build/rocket-os.img -d int,cpu_reset
    make test

When a boot error is added, keep its message visible in the blue VGA screen and include the failing stage (BPB, FAT, directory, or kernel load). This makes failures diagnosable without a serial debugger.
