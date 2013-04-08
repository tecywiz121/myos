extern kmain

global kinit

STACKSIZE equ 0x1000

section .text

kinit:
    mov     esp, stack + STACKSIZE          ; Create a new stack in the higher half

set_gdt:
    lgdt    [GDTR]                          ; Load the new GDT
    jmp     0x08:.ReloadSegments            ; Far jump to activate the new segments

    .ReloadSegments:
    ; Load the new data segment
    mov     ax, 0x10
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    mov     ss, ax
    jmp     $
    call    kmain

section .data

GDTR:                                       ; GDT Pointer structure
    dw      GDT_End - GDT - 1               ; 16-bit length of GDT
GDT_Ptr:
    dd      GDT                             ; 32-bit pointer to GDT
GDT:
    dd      0x00000000, 0x00000000          ; GDT[0] - Null GDT entry
    dd      0x0000FFFF, 0x00CF9C00          ; GDT[1] - Code segment
    dd      0x0000FFFF, 0x00CF9200          ; GDT[2] - Data segment
GDT_End:

section .bss

align 4
stack: resb STACKSIZE      ; Reserve a new stack that resides above KERNEL_BASE
