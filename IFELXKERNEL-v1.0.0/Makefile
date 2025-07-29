PROJECT = IFelxOS
CC = gcc
LD = i386-elf-ld
AS = nasm
CFLAGS = -m32 -ffreestanding -O2 -Wall -Wextra
LDFLAGS = -m elf_i386 -T linker.ld
OBJS = kernel_entry.o kernel.o vga.o keyboard.o commands.o mini_string.o heap.o

all: kernel.bin

kernel_entry.o: kernel_entry.asm
	$(AS) -f elf32 $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel.bin: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

iso: kernel.bin grub.cfg
	mkdir -p iso/boot/grub
	cp kernel.bin iso/boot/
	cp grub.cfg iso/boot/grub/
	grub-mkrescue -o $(PROJECT).iso iso

clean:
	rm -rf *.o kernel.bin iso $(PROJECT).iso

.PHONY: all iso clean
