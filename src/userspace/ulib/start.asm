; ============================================================================
; BOXOS USER PROGRAM STARTUP CODE
; ============================================================================
; This is the entry point for all user-space C programs.
; It sets up the environment and calls main().
; ============================================================================

BITS 64

section .text
global _start
extern main

_start:
    ; Clear direction flag
    cld

    ; Align stack to 16 bytes (ABI requirement)
    and rsp, ~0xF

    ; Call main()
    ; main() takes no arguments in BoxOS
    xor rdi, rdi    ; argc = 0
    xor rsi, rsi    ; argv = NULL
    call main

    ; If main returns, exit
    ; kernel_notify(0, NOTIFY_EXIT)
    mov rdi, 0      ; workflow_id = 0
    mov rsi, 0x10   ; NOTIFY_EXIT = 0x10
    int 0x80

    ; Should never reach here
.halt:
    hlt
    jmp .halt
