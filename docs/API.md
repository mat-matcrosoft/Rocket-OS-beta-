# ROCKET-OS kernel API

The public declarations live in api/api.h. Applications should use the rocket_* names instead of calling interrupts directly.

- rocket_print / rocket_println: console output.
- rocket_open, rocket_read, rocket_write: file handles and byte I/O.
- rocket_malloc / rocket_free: kernel heap boundary.

Syscall numbers are defined next to their wrappers in api/syscalls.c. The ABI uses interrupt 0x80 with the number in EAX and up to three arguments in EBX, ECX, and EDX. This is a starter ABI; validation and privilege transitions are required before production use.
