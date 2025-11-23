// ============================================================================
// BOXOS ELF64 LOADER
// ============================================================================

#ifndef ELF_LOADER_H
#define ELF_LOADER_H

#include "ktypes.h"
#include "elf.h"

// ============================================================================
// LOADER ERROR CODES
// ============================================================================

#define ELF_OK              0
#define ELF_ERR_NULL        1   // NULL pointer
#define ELF_ERR_NOT_ELF     2   // Not an ELF file
#define ELF_ERR_NOT_64      3   // Not 64-bit
#define ELF_ERR_NOT_X86_64  4   // Not x86-64
#define ELF_ERR_NOT_EXEC    5   // Not executable
#define ELF_ERR_NO_SEGMENTS 6   // No loadable segments
#define ELF_ERR_MEMORY      7   // Memory allocation failed
#define ELF_ERR_LOAD        8   // Failed to load segment

// ============================================================================
// LOADED ELF INFO
// ============================================================================

typedef struct {
    uint64_t entry_point;     // Entry point address
    uint64_t base_addr;       // Lowest loaded address
    uint64_t end_addr;        // Highest loaded address + 1
    uint64_t total_size;      // Total memory size
    uint32_t segment_count;   // Number of loaded segments
    uint32_t flags;           // Flags (executable type, etc.)
} ElfLoadInfo;

// Flags
#define ELF_FLAG_PIE     0x01  // Position-independent executable
#define ELF_FLAG_STATIC  0x02  // Statically linked

// ============================================================================
// LOADER API
// ============================================================================

// Validate ELF header (quick check)
int elf_validate(const void* data, size_t size);

// Get ELF info without loading
int elf_get_info(const void* data, size_t size, ElfLoadInfo* info);

// Load ELF into memory at specified base address
// Returns entry point on success, 0 on failure
uint64_t elf_load(const void* data, size_t size, uint64_t load_base, ElfLoadInfo* info);

// Load ELF into new process address space
// Uses VMM to allocate pages and map segments
uint64_t elf_load_process(const void* data, size_t size, void* vmm_context, ElfLoadInfo* info);

// Get human-readable error message
const char* elf_error_string(int error);

#endif // ELF_LOADER_H
