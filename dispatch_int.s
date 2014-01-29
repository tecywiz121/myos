;
;   Interrupts
;

isr_dispatch:
    pusha

    mov     ax, ds                          ; Get the data segment
    push    eax                             ; Push it onto the stack

    ; Load the kernel data segment
    mov     ax, 0x10
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    mov     ss, ax

    jmp $

    pop     eax                             ; Retrieve the original data segment
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    mov     ss, ax

    popa                                    ; Restore the pushed state
    add     esp, 8                          ; Clean up the pushed error code and interrup number
    sti
    iret

; Macro to create an interrupt handler for interrupts without error code
%macro ISR_NOERR 1
    isr%1:
        cli
        push byte 0                         ; Dummy error code
        push byte %1                        ; The interrupt number
        jmp isr_dispatch                    ; Jump to the common interrupt handler
%endmacro

; Macro to create an interrupt handler for interrupts with an error code
%macro ISR_ERR 1
    isr%1:
        cli
        push byte %1                        ; Push the interrupt number
        jmp isr_dispatch
%endmacro

; 0-7
%assign ii 0
%rep 8
    ISR_NOERR ii
    %assign ii ii+1
%endrep

; 8
ISR_ERR ii
%assign ii ii+1

; 9
ISR_NOERR ii
%assign ii ii+1

; 10-14
%rep 5
    ISR_ERR ii
    %assign ii ii+1
%endrep

; 15-31
%rep 17
    ISR_NOERR ii
    %assign ii ii+1
%endrep
