ARCH = x86_64
TARGET = ../kernel.elf
KERNEL_SRC = kernel.c print.c

all: $(TARGET)

$(TARGET): $(KERNEL_SRC)
	clang -target $(ARCH)-elf -ffreestanding -fno-stack-protector -fno-stack-check -D__$(ARCH)__ -I./uefi -c kernel.c -o kernel.o
	clang -target $(ARCH)-elf -ffreestanding -fno-stack-protector -fno-stack-check -D__$(ARCH)__ -I./uefi -c print.c -o print.o
	ld.lld -nostdlib -z max-page-size=0x1000 -Ttext=0x01000000 kernel.o print.o -o $(TARGET)


clean:
	rm -rf *.o $(TARGET)


