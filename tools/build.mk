AS := nasm
CC := i686-elf-gcc
LD := i686-elf-ld
OBJCOPY := i686-elf-objcopy
HOST_CC ?= cc
QEMU := qemu-system-i386
CFLAGS ?= -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -Wall -Wextra -I.
LDFLAGS ?= -m elf_i386 -T tools/linker.ld
BUILD := build

KERNEL_C := kernel/kernel.c kernel/vga.c kernel/screen.c kernel/shell.c drivers/device.c drivers/driver.c drivers/ata.c fs/fat32.c fs/fat32_fs.c rbe/rbe.c
KERNEL_S := kernel/entry.S kernel/syscall.S
KERNEL_O := $(patsubst %.c,$(BUILD)/%.o,$(KERNEL_C)) $(patsubst %.S,$(BUILD)/%.o,$(KERNEL_S))

$(BUILD):
	mkdir -p $(BUILD)/boot $(BUILD)/stage $(BUILD)/kernel $(BUILD)/drivers $(BUILD)/fs $(BUILD)/rbe

$(BUILD)/%.o: %.c | $(BUILD)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: %.S | $(BUILD)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/boot/boot.bin: boot/boot.asm | $(BUILD)
	$(AS) -f bin $< -o $@

$(BUILD)/stage/stage1.bin: stage/stage1.asm | $(BUILD)
	$(AS) -f bin $< -o $@

$(BUILD)/stage/stage1.pad: $(BUILD)/stage/stage1.bin
	dd if=$< of=$@ bs=4096 conv=sync status=none

$(BUILD)/kernel/kernel.elf: $(KERNEL_O) tools/linker.ld
	$(LD) $(LDFLAGS) -o $@ $(KERNEL_O)

$(BUILD)/kernel/kernel.bin: $(BUILD)/kernel/kernel.elf
	$(OBJCOPY) -O binary $< $@

$(BUILD)/rocket-os.img: $(BUILD)/boot/boot.bin $(BUILD)/stage/stage1.pad $(BUILD)/kernel/kernel.bin
	cat $^ > $@
	truncate -s $$((128 * 512)) $@

.PHONY: all image run clean test
all: image
image: $(BUILD)/rocket-os.img
run: image
	$(QEMU) -drive format=raw,file=$(BUILD)/rocket-os.img
clean:
	rm -rf $(BUILD)
test: $(BUILD)
	$(HOST_CC) -std=c11 -Wall -Wextra -I. tests/test_fat32.c fs/fat32.c fs/fat32_fs.c drivers/device.c -o $(BUILD)/test_fat32
	$(BUILD)/test_fat32
	$(HOST_CC) -std=c11 -Wall -Wextra -I. tests/test_devices.c drivers/device.c drivers/driver.c -o $(BUILD)/test_devices
	$(BUILD)/test_devices
	$(HOST_CC) -std=c11 -Wall -Wextra -I. tests/test_shell.c kernel/shell.c -o $(BUILD)/test_shell
	$(BUILD)/test_shell
