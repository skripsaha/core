#include "vga.h"
#include "klib.h"
#include "fpu.h"
#include "cpu.h"
#include "e820.h"
#include "vmm.h"
#include "pmm.h"
#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "ata.h"
#include "tagfs.h"
#include "pit.h"
#include "keyboard.h"
#include "eventdriven_system.h"
#include "serial.h"
#include "process.h"
#include "workflow.h"
#include "events.h"
#include "elf_loader.h"

// Linker-provided symbols for BSS section
extern char __bss_start[];
extern char __bss_end[];

// kernel_main receives parameters from bootloader via kernel_entry.asm:
// RDI = E820 map address (0x500)
// RSI = E820 entry count
// RDX = available memory start (0x100000)
void kernel_main(e820_entry_t* e820_map, uint64_t e820_count, uint64_t mem_start) {
    // ========================================================================
    // PHASE 0: BSS ZEROING (CRITICAL - must be first!)
    // ========================================================================
    // Use linker-provided symbols for BSS (automatic, no manual updates needed)
    // This ensures ALL BSS is cleared, regardless of kernel size

    volatile uint64_t* bss_ptr = (volatile uint64_t*)__bss_start;
    volatile uint64_t* bss_end = (volatile uint64_t*)__bss_end;

    // Zero BSS in 8-byte chunks for efficiency (may be several MB)
    while (bss_ptr < bss_end) {
        *bss_ptr++ = 0;
    }

    // NOTE: BSS cleared - all global variables now zeroed
    // (ring buffers, workflow contexts, decks, routing table, tagfs_storage)

    // ========================================================================
    // PHASE 1: EARLY INITIALIZATION
    // ========================================================================

    serial_init();
    serial_print("Kernel Workflow Engine: Initializing...\n");

    vga_init();
    kprintf("Kernel Workflow Engine Starting...\n");
    kprintf("Production Build - v1.0\n\n");

    // ========================================================================
    // PHASE 2: CORE HARDWARE INITIALIZATION
    // ========================================================================

    kprintf("Initializing core systems...\n\n");

    kprintf("[1] Enabling FPU...\n");
    enable_fpu();
    kprintf("[1] OK\n");

    kprintf("[2] E820 map (%lu entries)...\n", e820_count);
    e820_set_entries(e820_map, e820_count);
    kprintf("[2] OK\n");

    kprintf("[3] Physical memory manager...\n");
    pmm_init();
    kprintf("[3] OK\n");

    kprintf("[4] Memory allocator (from PMM)...\n");
    mem_init();
    kprintf("[4] OK\n");

    kprintf("[5] Virtual memory manager...\n");
    vmm_init();
    vmm_test_basic();  // TEMP: Disabled - causing panic
    kprintf("[5] OK\n");

    // ========================================================================
    // PHASE 3: STORAGE SYSTEM
    // ========================================================================

    kprintf("\n=== Storage System ===\n");
    kprintf("[6] ATA disk driver...\n");
    ata_init();
    kprintf("[6] OK\n");

    kprintf("[7] TagFS filesystem...\n");
    tagfs_init();
    kprintf("[7] OK\n");

    // ========================================================================
    // PHASE 4: CPU PROTECTION & INTERRUPTS
    // ========================================================================

    kprintf("\n=== CPU Protection & Interrupts ===\n");

    kprintf("[8] GDT (Kernel + User segments)...\n");
    gdt_init();
    kprintf("[8] OK\n");

    kprintf("[9] IDT (256 vectors)...\n");
    idt_init();
    kprintf("[9] OK\n");

    kprintf("[10] TSS (IST stacks)...\n");
    tss_init();
    kprintf("[10] OK\n");

    kprintf("[11] PIC (IRQs remapped)...\n");
    pic_init();
    kprintf("[11] OK\n");

    kprintf("[12] PIT timer (100 Hz)...\n");
    pit_init(100);  // 100 Hz = 10ms per tick
    kprintf("[12] OK\n");

    // ========================================================================
    // PHASE 5: EVENT-DRIVEN WORKFLOW SYSTEM
    // ========================================================================

    kprintf("\n=== Event-Driven Workflow System ===\n");
    kprintf("[13] Initializing event-driven system...\n");
    eventdriven_system_init();
    eventdriven_system_start();
    kprintf("[13] OK\n");

    kprintf("[14] Initializing workflow engine...\n");
    extern void workflow_engine_init(void);
    workflow_engine_init();
    kprintf("[14] OK - Workflow Engine ready!\n");

    kprintf("[15] Initializing process management...\n");
    process_init();
    kprintf("[15] OK - Process system ready!\n");

    kprintf("[16] Initializing scheduler...\n");
    extern void scheduler_init(void);
    scheduler_init();
    kprintf("[16] OK - Scheduler ready!\n");

    // ========================================================================
    // PHASE 6: ENABLE INTERRUPTS
    // ========================================================================

    kprintf("\n=== System Ready ===\n");
    kprintf("All core systems initialized successfully!\n");

    vga_clear_screen();

    kprintf("\n");
    kprintf("=================================================================\n");
    kprintf("         Kernel Workflow Engine - Production Ready              \n");
    kprintf("=================================================================\n");
    kprintf("\n");

    cpu_print_detailed_info();

    kprintf("\nSystem is ready to process workflows!\n");
    kprintf("NOTE: Interrupts will be enabled AFTER process creation\n\n");

    // ========================================================================
    // PHASE 7: REGISTER TEST WORKFLOW
    // ========================================================================

    kprintf("\n=== Registering Test Workflow ===\n");

    // Create a simple workflow that user program can activate
    WorkflowNode test_nodes[1];
    test_nodes[0].type = EVENT_TIMER_CREATE;  // Simple timer event
    test_nodes[0].data_size = 0;
    test_nodes[0].dependency_count = 0;
    test_nodes[0].ready = 1;
    test_nodes[0].completed = 0;
    test_nodes[0].error = 0;

    // Route: Operations Deck → Execution Deck
    uint8_t route[MAX_ROUTING_STEPS] = {1, 0, 0, 0, 0, 0, 0, 0};

    uint64_t workflow_id = workflow_register("test_workflow", route, 1, test_nodes, 0);
    if (workflow_id) {
        kprintf("[WORKFLOW] Registered test workflow: ID=%lu\n", workflow_id);
    } else {
        panic("Failed to register test workflow!");
    }

    // ========================================================================
    // PHASE 8: LAUNCH SHELL (Production Mode)
    // ========================================================================

    kprintf("\n=== Launching BoxOS Shell ===\n");

    // Load shell or test programs based on build configuration
    // Build with -DUSE_SHELL to enable shell, otherwise test programs
    #ifdef USE_SHELL
    #include "shell_binary.h"

    kprintf("[KERNEL] Loading shell (%u bytes ELF)...\n", shell_binary_len);

    // Validate ELF
    int elf_err = elf_validate(shell_binary, shell_binary_len);
    if (elf_err != ELF_OK) {
        kprintf("[KERNEL] ERROR: Shell ELF validation failed: %s\n",
                elf_error_string(elf_err));
        panic("Invalid shell ELF!");
    }

    // Create process from ELF
    process_t* shell_proc = process_create_elf(shell_binary, shell_binary_len);
    if (!shell_proc) {
        panic("Failed to create shell process!");
    }
    kprintf("[KERNEL] Shell process created (PID=%lu)\n", shell_proc->pid);

    // Add shell to scheduler
    extern void scheduler_add_process(process_t* proc);
    scheduler_add_process(shell_proc);

    kprintf("[KERNEL] Shell added to ready queue\n");
    kprintf("[KERNEL] Transitioning to Ring 3 (shell mode)...\n\n");

    #else
    // Fallback: Load test programs (default for development)
    kprintf("[KERNEL] Loading test programs (build with -DUSE_SHELL for shell)...\n");

    #include "user_storage_test_binary.h"
    #include "concurrent_test_binary.h"

    process_t* proc1 = process_create(user_storage_test_binary,
                                      user_storage_test_binary_len, 0);
    if (!proc1) {
        panic("Failed to create process 1!");
    }
    kprintf("[KERNEL] Process 1 created (PID=%lu)\n", proc1->pid);

    process_t* proc2 = process_create(concurrent_test_binary,
                                      concurrent_test_binary_len, 0);
    if (!proc2) {
        panic("Failed to create process 2!");
    }
    kprintf("[KERNEL] Process 2 created (PID=%lu)\n", proc2->pid);

    extern void scheduler_add_process(process_t* proc);
    scheduler_add_process(proc1);
    scheduler_add_process(proc2);

    kprintf("[KERNEL] Test processes added to ready queue\n");
    kprintf("[KERNEL] Transitioning to Ring 3...\n\n");
    #endif

    // CRITICAL: Enable interrupts NOW (processes are created and ready!)
    // Scheduler will automatically pick first process from ready queue on first timer tick
    kprintf("[KERNEL] Enabling interrupts - scheduler will pick first process...\n");
    asm volatile("sti");

    // Idle loop - scheduler handles everything from here via timer IRQ
    // First timer tick will pick a process from ready queue and start execution
    kprintf("[KERNEL] Entering idle loop - scheduler is now in control\n\n");
    while (1) {
        asm volatile("hlt");  // Wait for interrupts
    }

    // Should never reach here
    panic("Escaped from idle loop!");
}
