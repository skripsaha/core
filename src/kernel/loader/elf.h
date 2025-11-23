// ============================================================================
// ELF64 FORMAT DEFINITIONS
// ============================================================================

#ifndef ELF_H
#define ELF_H

#include "ktypes.h"

// ============================================================================
// ELF IDENTIFICATION
// ============================================================================

#define EI_NIDENT 16

// ELF Magic number
#define ELFMAG0 0x7F
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'

// ELF Class (32/64 bit)
#define ELFCLASSNONE 0
#define ELFCLASS32   1
#define ELFCLASS64   2

// ELF Data encoding
#define ELFDATANONE 0
#define ELFDATA2LSB 1  // Little-endian
#define ELFDATA2MSB 2  // Big-endian

// ELF Version
#define EV_NONE    0
#define EV_CURRENT 1

// ELF OS/ABI
#define ELFOSABI_NONE    0
#define ELFOSABI_SYSV    0
#define ELFOSABI_LINUX   3

// ============================================================================
// ELF FILE TYPES
// ============================================================================

#define ET_NONE   0      // No file type
#define ET_REL    1      // Relocatable
#define ET_EXEC   2      // Executable
#define ET_DYN    3      // Shared object
#define ET_CORE   4      // Core file

// ============================================================================
// ELF MACHINE TYPES
// ============================================================================

#define EM_NONE    0     // No machine
#define EM_386     3     // Intel 80386
#define EM_X86_64  62    // AMD x86-64

// ============================================================================
// PROGRAM HEADER TYPES
// ============================================================================

#define PT_NULL    0     // Unused
#define PT_LOAD    1     // Loadable segment
#define PT_DYNAMIC 2     // Dynamic linking info
#define PT_INTERP  3     // Interpreter path
#define PT_NOTE    4     // Auxiliary information
#define PT_SHLIB   5     // Reserved
#define PT_PHDR    6     // Program header table
#define PT_TLS     7     // Thread-local storage

// ============================================================================
// PROGRAM HEADER FLAGS
// ============================================================================

#define PF_X 0x1  // Execute
#define PF_W 0x2  // Write
#define PF_R 0x4  // Read

// ============================================================================
// SECTION HEADER TYPES
// ============================================================================

#define SHT_NULL     0   // Inactive
#define SHT_PROGBITS 1   // Program data
#define SHT_SYMTAB   2   // Symbol table
#define SHT_STRTAB   3   // String table
#define SHT_RELA     4   // Relocation with addends
#define SHT_HASH     5   // Symbol hash table
#define SHT_DYNAMIC  6   // Dynamic linking info
#define SHT_NOTE     7   // Notes
#define SHT_NOBITS   8   // BSS (no file data)
#define SHT_REL      9   // Relocation without addends
#define SHT_SHLIB    10  // Reserved
#define SHT_DYNSYM   11  // Dynamic symbol table

// ============================================================================
// SECTION HEADER FLAGS
// ============================================================================

#define SHF_WRITE     0x1   // Writable
#define SHF_ALLOC     0x2   // Occupies memory
#define SHF_EXECINSTR 0x4   // Executable

// ============================================================================
// ELF64 HEADER
// ============================================================================

typedef struct {
    uint8_t  e_ident[EI_NIDENT];  // ELF identification
    uint16_t e_type;              // Object file type
    uint16_t e_machine;           // Machine type
    uint32_t e_version;           // Object file version
    uint64_t e_entry;             // Entry point address
    uint64_t e_phoff;             // Program header offset
    uint64_t e_shoff;             // Section header offset
    uint32_t e_flags;             // Processor-specific flags
    uint16_t e_ehsize;            // ELF header size
    uint16_t e_phentsize;         // Program header entry size
    uint16_t e_phnum;             // Number of program headers
    uint16_t e_shentsize;         // Section header entry size
    uint16_t e_shnum;             // Number of section headers
    uint16_t e_shstrndx;          // Section name string table index
} __attribute__((packed)) Elf64_Ehdr;

// ============================================================================
// ELF64 PROGRAM HEADER
// ============================================================================

typedef struct {
    uint32_t p_type;    // Segment type
    uint32_t p_flags;   // Segment flags
    uint64_t p_offset;  // Offset in file
    uint64_t p_vaddr;   // Virtual address in memory
    uint64_t p_paddr;   // Physical address (unused)
    uint64_t p_filesz;  // Size in file
    uint64_t p_memsz;   // Size in memory
    uint64_t p_align;   // Alignment
} __attribute__((packed)) Elf64_Phdr;

// ============================================================================
// ELF64 SECTION HEADER
// ============================================================================

typedef struct {
    uint32_t sh_name;       // Section name (string table index)
    uint32_t sh_type;       // Section type
    uint64_t sh_flags;      // Section flags
    uint64_t sh_addr;       // Virtual address
    uint64_t sh_offset;     // Offset in file
    uint64_t sh_size;       // Section size
    uint32_t sh_link;       // Link to another section
    uint32_t sh_info;       // Additional info
    uint64_t sh_addralign;  // Alignment
    uint64_t sh_entsize;    // Entry size (if table)
} __attribute__((packed)) Elf64_Shdr;

// ============================================================================
// ELF64 SYMBOL TABLE ENTRY
// ============================================================================

typedef struct {
    uint32_t st_name;   // Symbol name (string table index)
    uint8_t  st_info;   // Symbol type and binding
    uint8_t  st_other;  // Symbol visibility
    uint16_t st_shndx;  // Section index
    uint64_t st_value;  // Symbol value
    uint64_t st_size;   // Symbol size
} __attribute__((packed)) Elf64_Sym;

// ============================================================================
// ELF64 RELOCATION ENTRIES
// ============================================================================

typedef struct {
    uint64_t r_offset;  // Address to relocate
    uint64_t r_info;    // Relocation type and symbol index
} __attribute__((packed)) Elf64_Rel;

typedef struct {
    uint64_t r_offset;  // Address to relocate
    uint64_t r_info;    // Relocation type and symbol index
    int64_t  r_addend;  // Addend
} __attribute__((packed)) Elf64_Rela;

// Relocation info macros
#define ELF64_R_SYM(i)  ((i) >> 32)
#define ELF64_R_TYPE(i) ((i) & 0xFFFFFFFF)

// x86-64 relocation types
#define R_X86_64_NONE     0
#define R_X86_64_64       1   // Direct 64-bit
#define R_X86_64_PC32     2   // PC relative 32-bit
#define R_X86_64_GOT32    3   // 32-bit GOT entry
#define R_X86_64_PLT32    4   // 32-bit PLT address
#define R_X86_64_RELATIVE 8   // Adjust by base

// ============================================================================
// ELF VALIDATION MACROS
// ============================================================================

#define IS_ELF(ehdr) \
    ((ehdr)->e_ident[0] == ELFMAG0 && \
     (ehdr)->e_ident[1] == ELFMAG1 && \
     (ehdr)->e_ident[2] == ELFMAG2 && \
     (ehdr)->e_ident[3] == ELFMAG3)

#define IS_ELF64(ehdr) \
    ((ehdr)->e_ident[4] == ELFCLASS64)

#define IS_ELF_X86_64(ehdr) \
    ((ehdr)->e_machine == EM_X86_64)

#define IS_ELF_EXEC(ehdr) \
    ((ehdr)->e_type == ET_EXEC || (ehdr)->e_type == ET_DYN)

#endif // ELF_H
