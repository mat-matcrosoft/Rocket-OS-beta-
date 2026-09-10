# Storage stack

## Device and driver managers

The drivers/device.h interface defines the block-device ABI. A device exposes a name, sector size/count, context, and read/write callbacks. The registry currently holds eight devices and rejects duplicate names or out-of-range I/O.

The drivers/driver.h interface defines a small driver registry. Drivers register a probe callback; driver_manager_probe_all() invokes registered probes during kernel startup. The ATA driver uses this callback to discover disks and register them as ata0 and ata1.

## ATA PIO

drivers/ata.c supports the primary IDE channel at 0x1F0/0x3F6, master and slave devices, IDENTIFY, LBA28 PIO reads, and cache-flushed writes. All status waits are bounded. The implementation intentionally does not claim LBA48 or secondary-channel support yet.

## FAT32

fs/fat32.c mounts a 512-byte FAT32 superfloppy, validates the BPB and volume bounds, follows FAT chains with a cluster-visit limit, ignores deleted/LFN/volume-label entries, and exposes short 8.3 directory lookup.

Available operations are directory listing, file reads, and replacement writes to an existing file when its current cluster chain is large enough. Writes update file contents and directory size but do not allocate or free clusters. LFN checksum validation, path creation, cluster allocation, journaling/cache flush, MBR partition offsets, and FAT32 Stage 2 BIOS loading remain separate milestones.

## Kernel integration

Kernel startup initializes both registries, probes ATA, mounts ata0 at LBA 0, and exposes devices, ls, and cat in the shell. A non-FAT32 development image reports a readable mount error and continues to the shell.

## Tests

make test runs a memory-backed FAT32 test (mount, list, read, write, reread), a device/driver registry test, and the existing shell test. The tests do not execute privileged ATA port I/O.
