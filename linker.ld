KERNEL_LOW  = 0x00100000;
KERNEL_BASE = 0xC0000000;

OUTPUT_FORMAT(elf32-i386)
ENTRY(bootstrap)

SECTIONS
{
   . = KERNEL_LOW;   /* Low Memory Address = 2Mb */

   .bootstrap : AT(ADDR(.bootstrap)) {
      *(.multiboot)
      . = ALIGN(0x1000);
      bootstrap.o (.text)
      bootstrap.o (.rodata)
      . = ALIGN(0x1000);
      bootstrap.o (.data)
      . = ALIGN(0x1000);
      bootstrap.o (.bss)
      . = ALIGN(0x1000);
      bootstrap.o (.ehframe)
   }

   . += KERNEL_BASE;
   . = ALIGN(0x1000);
   _start = .;
   _start_pa = . - KERNEL_BASE;

   .text : AT(ADDR(.text) - KERNEL_BASE) {
      _code = .;
      *(EXCLUDE_FILE(*bootstrap.o) .text)
      *(EXCLUDE_FILE(*bootstrap.o) .rodata*)
      . = ALIGN(0x1000);
   }

   .data : AT(ADDR(.data) - KERNEL_BASE) {
      _data = .;
      *(EXCLUDE_FILE(*bootstrap.o) .data)
      . = ALIGN(4096);
   }

   .ehframe : AT(ADDR(.ehframe) - KERNEL_BASE)   {
      _ehframe = .;
      *(EXCLUDE_FILE(*bootstrap.o) .ehframe)
      . = ALIGN(0x1000);
   }

   .bss : AT(ADDR(.bss) - KERNEL_BASE)   {
      _bss = .;
      *(COMMON)
      *(EXCLUDE_FILE(*bootstrap.o) .bss)
      . = ALIGN(0x1000);
   }

   _end = .;
   _end_pa = . - KERNEL_BASE;

   /DISCARD/ :
   {
      *(.comment)
   }
}
