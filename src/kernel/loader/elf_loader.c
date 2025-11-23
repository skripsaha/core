// ============================================================================
// BOXOS ELF64 LOADER - Implementation
// ============================================================================

#include "elf_loader.h"
#include "elf.h"
#include "klib.h"
#include "vmm.h"

// ============================================================================
// ERROR MESSAGES
// ============================================================================

static const char* error_messages[] = {
    "OK",
    "NULL pointer",
    "Not an ELF file",
    "Not 64-bit ELF",
    "Not x86-64 architecture",
    "Not an executable",
    "No loadable segments",
    "Memory allocation failed",
    "Failed to load segment"
};

const char* elf_error_string(int error) {
    if (error < 0 || error > ELF_ERR_LOAD) {
        return "Unknown error";
    }
    return error_messages[error];
}

// ============================================================================
// VALIDATION
// ============================================================================

int elf_validate(const void* data, size_t size) {
    if (!data) {
        return ELF_ERR_NULL;
    }

    // Minimum size check
    if (size < sizeof(Elf64_Ehdr)) {
        return ELF_ERR_NOT_ELF;
    }

    const Elf64_Ehdr* ehdr = (const Elf64_Ehdr*)data;

    // Check magic number
    if (!IS_ELF(ehdr)) {
        return ELF_ERR_NOT_ELF;
    }

    // Check 64-bit
    if (!IS_ELF64(ehdr)) {
        return ELF_ERR_NOT_64;
    }

    // Check x86-64
    if (!IS_ELF_X86_64(ehdr)) {
        return ELF_ERR_NOT_X86_64;
    }

    // Check executable
    if (!IS_ELF_EXEC(ehdr)) {
        return ELF_ERR_NOT_EXEC;
    }

    return ELF_OK;
}

// ============================================================================
// GET INFO
// ============================================================================

int elf_get_info(const void* data, size_t size, ElfLoadInfo* info) {
    int err = elf_validate(data, size);
    if (err != ELF_OK) {
        return err;
    }

    if (!info) {
        return ELF_ERR_NULL;
    }

    const Elf64_Ehdr* ehdr = (const Elf64_Ehdr*)data;

    // Initialize info
    memset(info, 0, sizeof(ElfLoadInfo));
    info->entry_point = ehdr->e_entry;
    info->base_addr = UINT64_MAX;
    info->end_addr = 0;

    // Set flags based on ELF type
    if (ehdr->e_type == ET_DYN) {
        info->flags |= ELF_FLAG_PIE;
    }

    // Scan program headers
    const uint8_t* phdr_base = (const uint8_t*)data + ehdr->e_phoff;

    for (uint16_t i = 0; i < ehdr->e_phnum; i++) {
        const Elf64_Phdr* phdr = (const Elf64_Phdr*)(phdr_base + i * ehdr->e_phentsize);

        if (phdr->p_type == PT_LOAD) {
            info->segment_count++;

            // Track address range
            if (phdr->p_vaddr < info->base_addr) {
                info->base_addr = phdr->p_vaddr;
            }

            uint64_t seg_end = phdr->p_vaddr + phdr->p_memsz;
            if (seg_end > info->end_addr) {
                info->end_addr = seg_end;
            }
        }
    }

    if (info->segment_count == 0) {
        return ELF_ERR_NO_SEGMENTS;
    }

    info->total_size = info->end_addr - info->base_addr;

    return ELF_OK;
}

// ============================================================================
// LOAD ELF (Simple - into flat memory)
// ============================================================================

uint64_t elf_load(const void* data, size_t size, uint64_t load_base, ElfLoadInfo* info) {
    ElfLoadInfo local_info;
    if (!info) {
        info = &local_info;
    }

    int err = elf_get_info(data, size, info);
    if (err != ELF_OK) {
        kprintf("[ELF] Validation failed: %s\n", elf_error_string(err));
        return 0;
    }

    const Elf64_Ehdr* ehdr = (const Elf64_Ehdr*)data;
    const uint8_t* file_data = (const uint8_t*)data;

    // Calculate relocation offset for PIE
    int64_t reloc_offset = 0;
    if (info->flags & ELF_FLAG_PIE) {
        reloc_offset = (int64_t)load_base - (int64_t)info->base_addr;
    } else {
        // Non-PIE: load at specified addresses
        load_base = info->base_addr;
    }

    kprintf("[ELF] Loading %u segments (base=0x%lx, reloc=0x%lx)\n",
            info->segment_count, load_base, reloc_offset);

    // Load each PT_LOAD segment
    const uint8_t* phdr_base = file_data + ehdr->e_phoff;

    for (uint16_t i = 0; i < ehdr->e_phnum; i++) {
        const Elf64_Phdr* phdr = (const Elf64_Phdr*)(phdr_base + i * ehdr->e_phentsize);

        if (phdr->p_type != PT_LOAD) {
            continue;
        }

        // Calculate load address
        uint64_t load_addr = phdr->p_vaddr + reloc_offset;

        kprintf("[ELF]   Segment %u: vaddr=0x%lx -> load=0x%lx (filesz=%lu, memsz=%lu)\n",
                i, phdr->p_vaddr, load_addr, phdr->p_filesz, phdr->p_memsz);

        // Copy file data to memory
        if (phdr->p_filesz > 0) {
            memcpy((void*)load_addr, file_data + phdr->p_offset, phdr->p_filesz);
        }

        // Zero BSS (memsz > filesz)
        if (phdr->p_memsz > phdr->p_filesz) {
            memset((void*)(load_addr + phdr->p_filesz), 0,
                   phdr->p_memsz - phdr->p_filesz);
        }
    }

    // Calculate final entry point
    uint64_t entry = ehdr->e_entry + reloc_offset;

    kprintf("[ELF] Load complete. Entry point: 0x%lx\n", entry);

    // Update info with final addresses
    info->entry_point = entry;
    info->base_addr += reloc_offset;
    info->end_addr += reloc_offset;

    return entry;
}

// ============================================================================
// LOAD ELF INTO PROCESS (with VMM)
// ============================================================================

uint64_t elf_load_process(const void* data, size_t size, void* vmm_context, ElfLoadInfo* info) {
    ElfLoadInfo local_info;
    if (!info) {
        info = &local_info;
    }

    int err = elf_get_info(data, size, info);
    if (err != ELF_OK) {
        kprintf("[ELF] Validation failed: %s\n", elf_error_string(err));
        return 0;
    }

    if (!vmm_context) {
        kprintf("[ELF] No VMM context provided\n");
        return 0;
    }

    const Elf64_Ehdr* ehdr = (const Elf64_Ehdr*)data;
    const uint8_t* file_data = (const uint8_t*)data;

    // For PIE executables, we can relocate to any base
    // For regular executables, we must use the specified addresses
    uint64_t load_base = info->base_addr;
    int64_t reloc_offset = 0;

    if (info->flags & ELF_FLAG_PIE) {
        // Load PIE at user space base (e.g., 0x400000)
        load_base = 0x400000;
        reloc_offset = (int64_t)load_base - (int64_t)info->base_addr;
    }

    kprintf("[ELF] Loading into process (base=0x%lx, segments=%u)\n",
            load_base, info->segment_count);

    // Load each PT_LOAD segment
    const uint8_t* phdr_base = file_data + ehdr->e_phoff;

    for (uint16_t i = 0; i < ehdr->e_phnum; i++) {
        const Elf64_Phdr* phdr = (const Elf64_Phdr*)(phdr_base + i * ehdr->e_phentsize);

        if (phdr->p_type != PT_LOAD) {
            continue;
        }

        // Calculate load address
        uint64_t seg_vaddr = phdr->p_vaddr + reloc_offset;
        uint64_t seg_size = phdr->p_memsz;

        // Align to page boundaries
        uint64_t page_start = seg_vaddr & ~0xFFF;
        uint64_t page_end = (seg_vaddr + seg_size + 0xFFF) & ~0xFFF;
        size_t page_count = (page_end - page_start) / 4096;

        // Determine VMM flags from segment flags
        uint64_t vmm_flags = VMM_FLAGS_USER_RO;  // User accessible, read-only
        if (phdr->p_flags & PF_W) {
            vmm_flags = VMM_FLAGS_USER_RW;  // User accessible, writable
        }
        // Note: NX bit is handled separately if CPU supports it

        // Allocate pages for this segment
        void* pages = vmm_alloc_pages(vmm_context, page_count, vmm_flags);
        if (!pages) {
            kprintf("[ELF] Failed to allocate %lu pages for segment\n", page_count);
            return 0;
        }

        // Map pages at the correct virtual address
        // Note: vmm_alloc_pages returns the virtual address, so we need to
        // ensure it's at the right location or use vmm_map_pages instead

        // For now, copy data to allocated pages
        // (This assumes vmm_alloc_pages returns usable kernel-mapped memory)

        uint64_t offset_in_page = seg_vaddr - page_start;

        // Zero the pages first
        memset(pages, 0, page_count * 4096);

        // Copy file data
        if (phdr->p_filesz > 0) {
            memcpy((uint8_t*)pages + offset_in_page,
                   file_data + phdr->p_offset,
                   phdr->p_filesz);
        }

        kprintf("[ELF]   Segment: 0x%lx-%lx (%lu pages, flags=%x)\n",
                seg_vaddr, seg_vaddr + seg_size, page_count, phdr->p_flags);
    }

    // Calculate final entry point
    uint64_t entry = ehdr->e_entry + reloc_offset;

    // Update info
    info->entry_point = entry;
    info->base_addr = load_base;
    info->end_addr = load_base + info->total_size;

    kprintf("[ELF] Process load complete. Entry: 0x%lx\n", entry);

    return entry;
}
