# ROCKET-OS design

## Milestone 0.1

The repository is a deliberately small 16/32-bit x86 foundation. BIOS stage 0 draws the white ROCKET-OS logo on a blue VGA text screen, reads a bounded contiguous payload, and transfers control to stage 1. Stage 1 installs a flat GDT and enters protected mode. The C kernel then owns the VGA console and a polling PS/2 keyboard shell.

The contiguous read is intentional for the first bootable milestone. FAT32 block I/O and directory code are isolated in fs/ so the next loader can replace the fixed-sector read without changing the kernel ABI.

## Layers

- boot/: BIOS stage 0 and splash screen.
- stage/: protected-mode transition metadata.
- kernel/: entry point, VGA driver, shell, and syscall entry placeholder.
- fs/: packed FAT32 boot-sector model and cluster arithmetic.
- rbe/: RBE1 header, section table, bounds validation.
- api/: application-facing syscall wrappers.

## RBE format

The file begins with RbeHeader: magic RBE1, uint16 version, uint16 section count, uint32 entry offset, and a checksum field reserved for the next verifier milestone. It is followed by section records containing type, offset, size, and an eight-byte name. CODE and DATA are loadable; BSS reserves memory without file bytes.

## Next milestones

1. Replace the contiguous boot read with a FAT32 stage-2 loader.
2. Add a block-device abstraction and LFN-aware directory traversal.
3. Implement kernel heap and syscall dispatch with user-mode protection.
4. Add an RBE builder and a QEMU disk image populated with applications.
