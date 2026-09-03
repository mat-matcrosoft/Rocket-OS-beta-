# ROCKET-OS design

## Milestone 0.1

The repository is a deliberately small 16/32-bit x86 foundation. BIOS stage 0 draws the white ROCKET-OS logo on a blue VGA text screen, reads a bounded contiguous payload, and transfers control to stage 1. Stage 1 installs a flat GDT and enters protected mode. The C kernel then owns the VGA console and a polling PS/2 keyboard shell.

The contiguous read is intentional for the first bootable milestone. FAT32 block I/O and directory code are isolated in fs/ so the next loader can replace the fixed-sector read without changing the kernel ABI. The current raw image is therefore a bootable development image, not a FAT32 volume.

## Build pipeline

The source pipeline is boot/boot.asm -> stage/stage1.asm -> kernel/entry.S and C objects -> tools/linker.ld -> build/kernel/kernel.bin -> build/rocket-os.img. Stage 1 is padded to 4 KiB, so the kernel is linked and loaded at physical address 0x11000. Stage 0 currently reads a bounded payload from sector 2 onward.

The FAT32 pipeline will replace the last step with a filesystem image containing SYSTEM/KERNEL.BIN. Stage 0 should load a compact Stage 2 reader; Stage 2 should parse the BPB and load the kernel by cluster chain. Kernel code must not depend on BIOS services after protected mode starts.

## FAT32 loading contract

The loader reads a 512-byte boot sector and validates the BPB before using any derived address. The essential values are bytes per sector, sectors per cluster, reserved sectors, FAT count, sectors per FAT, and root cluster. All arithmetic must use a wide integer type and reject overflow.

For a data cluster N:

    fat_begin = reserved_sectors
    data_begin = reserved_sectors + fats * sectors_per_fat
    cluster_lba(N) = data_begin + (N - 2) * sectors_per_cluster

A FAT32 entry is four bytes. Mask the high four bits, reject bad/reserved values, and stop at 0x0FFFFFF8 or above. Directory walking starts at root_cluster and later follows subdirectory clusters. Short names are the first implementation; LFN entries must validate their ordinal sequence and short-name checksum before being exposed to the shell.

The loader must cap the number of clusters, ensure every sector read is inside the image, detect loops, and keep the kernel buffer away from the Stage 1 stack/GDT and protected-mode stack. A bad image is a user-visible boot error, not an unchecked jump.

## Layers

- boot/: BIOS stage 0 and splash screen.
- stage/: protected-mode transition metadata; Stage 2 FAT32 reader is the next boot milestone.
- kernel/: entry point, VGA driver, shell, and syscall entry placeholder.
- fs/: packed FAT32 boot-sector model and cluster arithmetic.
- rbe/: RBE1 header, section table, and bounds validation.
- api/: application-facing syscall wrappers.

## RBE format

The file begins with RbeHeader: magic RBE1, uint16 version, uint16 section count, uint32 entry offset, and a checksum field reserved for the next verifier milestone. It is followed by section records containing type, offset, size, and an eight-byte name. CODE and DATA are loadable; BSS reserves memory without file bytes.

Before execution, the RBE loader must validate the whole header and section table, enforce non-overlapping sections, verify the checksum, ensure the entry point belongs to executable CODE, and reject addresses outside the application sandbox. The current parser only provides the initial bounds-checking scaffold.

## Milestones

1. Bootable contiguous alpha: blue splash, Stage 1, protected mode, VGA shell.
2. FAT32 Stage 2: block reads, BPB validation, FAT chains, short names, then LFN.
3. Kernel filesystem: read-only API, then safe writes with cache and recovery rules.
4. RBE toolchain: builder, checksum, section loader, and entry validation.
5. User mode: heap, syscall dispatch, GDT/TSS isolation, and application ABI.
6. QEMU regression suite: valid images plus malformed BPB, fragmented files, LFN, and invalid RBE cases.
