ARCH = x86_64
TARGET = BOOTX64.efi
SRCS = elfloader.c
KERNEL = kernel.elf

include uefi/Makefile


.PHONY: kernel

kernel:
	@make -C kernel all

my-all: all kernel image run

image:
	dd if=/dev/zero of=fat.img bs=1k count=1440
	mformat -i fat.img -f 1440 ::
	mmd -i fat.img ::/EFI
	mmd -i fat.img ::/EFI/BOOT
	mcopy -i fat.img $(TARGET) ::/EFI/BOOT
	mcopy -i fat.img $(KERNEL) ::/EFI/BOOT
	mkgpt -o hdimage.bin --image-size 4096 --part fat.img --type system

run:
	qemu-system-x86_64 -L ./ -pflash OVMF.fd -hda hdimage.bin -net none -vga std

my-clean: clean
	rm -rf $(TARGET) hdimage.bin fat.img