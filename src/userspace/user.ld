/* ============================================================================
 * BoxOS Userspace Linker Script
 * ============================================================================
 * ELF64 executable for user-space programs
 * Load address: 0x400000 (standard Linux user base)
 * ============================================================================ */

OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

SECTIONS
{
    /* Start at 4MB - standard user space base */
    . = 0x400000;

    /* Code section */
    .text : ALIGN(4096)
    {
        *(.text.entry)      /* Entry point first */
        *(.text .text.*)
    }

    /* Read-only data */
    .rodata : ALIGN(4096)
    {
        *(.rodata .rodata.*)
    }

    /* Initialized data */
    .data : ALIGN(4096)
    {
        *(.data .data.*)
    }

    /* Uninitialized data (BSS) */
    .bss : ALIGN(4096)
    {
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    }

    /* End of program */
    _end = .;

    /* Discard unwanted sections */
    /DISCARD/ :
    {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
    }
}
