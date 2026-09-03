# ROCKET-OS kernel API

The public declarations live in api/api.h. Applications should use the rocket_* names instead of calling interrupts directly.

## Console and input

- rocket_print / rocket_println: console output.
- Future getchar/getchar_nonblocking/putchar calls: keyboard and console primitives exposed through syscalls.

## Filesystem

The current API names are reserved for the FAT32-backed implementation:

- rocket_open(path, flags): open a file or directory and return a handle.
- rocket_read(fd, buffer, length): read bytes and return the count, or a negative error.
- rocket_write(fd, buffer, length): write bytes and return the count, or a negative error.
- Planned stat, opendir, readdir, closedir, mkdir, unlink, rename, and sync calls complete the shell filesystem contract.

The implementation must normalize paths, prevent traversal outside the mounted volume, validate handles, and return a stable error for a missing or malformed directory entry. Long File Names are a filesystem concern and must not leak raw VFAT slot records to applications.

## Memory and processes

- rocket_malloc / rocket_free: kernel heap boundary.
- Planned process_create and RBE loading calls: create an isolated user address space only after GDT/TSS and syscall validation are in place.

## Syscall ABI

Syscall numbers are defined next to their wrappers in api/syscalls.c. The ABI uses interrupt 0x80 with the number in EAX and up to three arguments in EBX, ECX, and EDX. This is a starter ABI; pointer validation, privilege transitions, errno-style errors, and syscall dispatch are required before production use.

## FAT32 loader boundary

The bootloader owns BIOS disk reads and loading KERNEL.BIN. After protected mode starts, the kernel should use a sector-device interface rather than BIOS calls:

    int disk_read(uint32_t lba, uint32_t count, void *buffer);

The FAT32 layer then consumes that interface for BPB parsing, cluster traversal, directory enumeration, and file I/O. A memory-backed disk implementation should be used by host tests.
