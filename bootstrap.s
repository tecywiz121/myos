global bootstrap
global _b_page_directory
global _b_PAGE_TABLES
global _b_multiboot_info
global _b_print

extern kinit
extern _start
extern _start_pa
extern _end_pa
extern _b_start
extern _b_end
extern KERNEL_BASE

section .text

STACKSIZE equ 0x1000                        ; Amount of memory to reserve for the stack

;
;   Bootstrap
;       The main entry point for the OS.  Sets up GDT, and calls kinit

bootstrap:
    cli                                     ; Disable interrupts

    mov     esp, stack + STACKSIZE          ; Create a stack for the bootstrap
    mov     [_b_magic], eax                 ; Store the multiboot magic number
    mov     [_b_mbd], ebx                   ; Store pointer to multiboot info structure

    push    msg_welcome                     ; Display a nice hello world
    call    _b_print
    add     esp, 4                          ; Clean up the stack

check_magic:
    cmp     DWORD [_b_magic], 0x2BADB002    ; Make sure the magic value is correct
    jne     bad_magic                       ; If its not, print a message and die

copy_multiboot:
    mov     ecx, 22                         ; 1/4 length of multiboot structure (4 bytes at a time)
    mov     eax, [_b_mbd]                   ; Get the location of the multiboot structure
    mov     ebx, _b_multiboot_info          ; Get the destination location
    .CopyLoop:
        mov     edx, [eax]
        mov     [ebx], edx
        add     eax, 4
        add     ebx, 4
        loop .CopyLoop

init_paging:
    ; Set up the page directory
    %assign n_pages 1                       ; The number of pages to build
    mov     cx, n_pages
    mov     ebx, _b_page_directory

    .DirectoryLoop:
        or      DWORD [ebx], 0x03           ; Set the present and read/write bits
        add     ebx, 4                      ; Advance to the next page pointer
        loop    .DirectoryLoop

    or DWORD [kernel_page], 0x03            ; Set the present bit on the kernel page

    jmp     $
    mov     cx, 256                         ; Identity map the first megabyte

    mov     ebx, _b_end
    sub     ebx, _b_start                   ; Calculate length of bootstrap
    shr     ebx, 12                         ; Divide by page size
    add     cx, bx                          ; Identity map that as well (assuming _b_end - _b_start fits in cx)

    mov     ebx, page_table0
    mov     edx, 1                          ; Bit 0 is present

    .IdentityMap:
        mov     [ebx], edx                  ; Put the address into the page table entry
        add     ebx, 4                      ; Advance to the next page table entry
        add     edx, 0x1000                 ; Advance to the next physical page
        loop    .IdentityMap

    mov     ebx, _start                     ; Get the start addres to calculate offset into page table
    sub     ebx, KERNEL_BASE                ; Subtract the BASE
    shr     ebx, 10                         ; Divide by (size of page) and multiply by size of entry
                                            ; to get the offset into the page table

    add     ebx, page_table768              ; Pointer to the kernel page table

    mov     edx, _start_pa                  ; Get the physical address of the kernel

    mov     ecx, _end_pa                    ; Get the physical address of the end of the kernel
    sub     ecx, edx                        ; Subtract the start address to get the length

    add     ecx, 4095                       ; Divide the length by 4096 and round up
    shr     ecx, 12                         ; To get the number of pages to map

    inc     edx                             ; Set the present bit
    .KernelMap:
        mov     [ebx], edx                  ; Put the address into the page table
        add     edx, 0x1000                 ; Advance the physical address by one page
        add     ebx, 4                      ; Advance the page table pointer
        loop .KernelMap

enable_paging:
    mov     eax, _b_page_directory
    mov     cr3, eax

    mov     eax, cr0
    or      eax, 0x80000000
    mov     cr0, eax

kernel:
    call    kinit

halt:
    ; Halt the machine if the kernel ever returns
    hlt
    jmp     halt

bad_magic:
    ; Inform the user about a bad magic value and die
    push    msg_bad_magic
    call    _b_print
    add     esp, 4
    jmp     halt

_b_print:
    ; print out a message
    mov     eax, [esp+4]                    ; Get pointer to message
    mov     ebx, 0xB8000                    ; Video Memory
    mov     ecx, 0
    .PrintLoop:
        mov     cl, [eax]
        cmp     cl, 0
        je      .Return
        mov     [ebx], cl
        mov     BYTE [ebx+1], 0x07
        add     ebx, 2
        inc     eax
        jmp     .PrintLoop
    .Return:
        ret

section .rodata
msg_welcome:    db  'hello, world', 0x0
msg_bad_magic:  db  'Bad Multiboot Magic Number', 0x0

section .data

align 0x1000
_b_page_directory:                          ; Array of pointers to page tables
%assign ii 0                                ; Store pointers to pages created below
%rep n_pages
    dd page_table%+ii
    %assign ii ii+1
%endrep

%assign fill 768-n_pages                    ; Fill up the rest of the page directory with zeros
%rep fill
    dd 0x0
%endrep
%undef fill

kernel_page: dd page_table768               ; Put in a page for the kernel

%rep 255                                    ; Fill the remaining entries
    dd 0x0
%endrep

section .bss

align 4
stack:  resb STACKSIZE
_b_magic:  resd 1                           ; Stores the multiboot magic number
_b_mbd:    resd 1                           ; Pointer to the multiboot info structure

align 4
_b_multiboot_info: resb 88                  ; Store a copy of the multiboot_info

;
; Page Data Structures
;

%macro PAGE_TABLE 1
    alignb 0x1000
    page_table%1: resb 4096
%endmacro

%assign ii 0
%rep n_pages
    PAGE_TABLE ii
    %assign ii ii+1
%endrep

PAGE_TABLE 768
