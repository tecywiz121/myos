NASM	= nasm
CC		= gcc -m32
CFLAGS	= -g -gdwarf-2 -Wall -Wextra -pedantic -std=c11 -ffreestanding -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -mno-sse3 -mno-3dnow -nostdlib -nostartfiles -nodefaultlibs -masm=intel
LD		= ld
LDFLAGS	= -T linker.ld -melf_i386 -g -nostdinc -nostdlib

OBJFILES	= multiboot.o bootstrap.o loader.o memmgr_physical.o kernel.o

all: kernel.bin

.s.o:
	$(NASM) -f elf32 -o $@ $<

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

kernel.bin: $(OBJFILES)
	$(LD) $(LDFLAGS) -o $@ $^

clean:
	$(RM) $(OBJFILES) kernel.bin bootable.iso
