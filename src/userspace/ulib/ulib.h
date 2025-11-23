// ============================================================================
// BOXOS USER LIBRARY - Minimal runtime for user-space programs
// ============================================================================
//
// This library provides the interface between user programs and the kernel
// through the event-driven workflow system. There is only ONE syscall:
// kernel_notify(workflow_id, flags)
//
// ============================================================================

#ifndef ULIB_H
#define ULIB_H

// ============================================================================
// BASIC TYPES (no stdlib!)
// ============================================================================

typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;
typedef signed long long   int64_t;
typedef uint64_t           size_t;

#define NULL ((void*)0)
#define true 1
#define false 0

// ============================================================================
// KERNEL NOTIFY FLAGS
// ============================================================================

#define NOTIFY_SUBMIT  0x01  // Process events from EventRing
#define NOTIFY_WAIT    0x02  // Block until workflow completes
#define NOTIFY_POLL    0x04  // Non-blocking check for results
#define NOTIFY_YIELD   0x08  // Yield CPU to other processes
#define NOTIFY_EXIT    0x10  // Exit process

// ============================================================================
// EVENT TYPES (must match kernel events.h)
// ============================================================================

// Console operations (Hardware Deck)
#define EVENT_CONSOLE_WRITE      70
#define EVENT_CONSOLE_WRITE_ATTR 71
#define EVENT_CONSOLE_READ_LINE  72
#define EVENT_CONSOLE_READ_CHAR  73
#define EVENT_CONSOLE_CLEAR      74
#define EVENT_CONSOLE_SET_POS    75
#define EVENT_CONSOLE_GET_POS    76

// File operations (Storage Deck)
#define EVENT_FILE_OPEN          10
#define EVENT_FILE_CLOSE         11
#define EVENT_FILE_READ          12
#define EVENT_FILE_WRITE         13
#define EVENT_FILE_STAT          14
#define EVENT_FILE_CREATE_TAGGED 15
#define EVENT_FILE_QUERY         16
#define EVENT_FILE_TAG_ADD       17
#define EVENT_FILE_TAG_REMOVE    18
#define EVENT_FILE_TAG_GET       19

// Timer operations (Hardware Deck)
#define EVENT_TIMER_SLEEP        52

// ============================================================================
// VGA COLOR ATTRIBUTES (must match kernel vga.h)
// ============================================================================

#define VGA_BLACK         0x0
#define VGA_BLUE          0x1
#define VGA_GREEN         0x2
#define VGA_CYAN          0x3
#define VGA_RED           0x4
#define VGA_MAGENTA       0x5
#define VGA_BROWN         0x6
#define VGA_LIGHT_GRAY    0x7
#define VGA_DARK_GRAY     0x8
#define VGA_LIGHT_BLUE    0x9
#define VGA_LIGHT_GREEN   0xA
#define VGA_LIGHT_CYAN    0xB
#define VGA_LIGHT_RED     0xC
#define VGA_LIGHT_MAGENTA 0xD
#define VGA_YELLOW        0xE
#define VGA_WHITE         0xF

#define VGA_ATTR(fg, bg) ((uint8_t)(((bg) << 4) | (fg)))

#define VGA_DEFAULT       VGA_ATTR(VGA_LIGHT_GRAY, VGA_BLACK)
#define VGA_ERROR         VGA_ATTR(VGA_LIGHT_RED, VGA_BLACK)
#define VGA_SUCCESS       VGA_ATTR(VGA_LIGHT_GREEN, VGA_BLACK)
#define VGA_WARNING       VGA_ATTR(VGA_YELLOW, VGA_BLACK)
#define VGA_HINT          VGA_ATTR(VGA_DARK_GRAY, VGA_BLACK)
#define VGA_PROMPT        VGA_ATTR(VGA_LIGHT_GREEN, VGA_BLACK)
#define VGA_PROMPT_TAG    VGA_ATTR(VGA_CYAN, VGA_BLACK)
#define VGA_INPUT         VGA_ATTR(VGA_WHITE, VGA_BLACK)

// ============================================================================
// RING BUFFER ADDRESSES (set by kernel in process_create)
// ============================================================================

#define EVENT_RING_ADDR   0x20200000
#define RESULT_RING_ADDR  0x202400A0

// ============================================================================
// EVENT STRUCTURE (256 bytes, must match kernel)
// ============================================================================

#define EVENT_DATA_SIZE 224
#define MAX_ROUTING_STEPS 8

typedef struct __attribute__((packed)) {
    uint64_t id;
    uint64_t user_id;       // workflow_id
    uint32_t type;
    uint32_t _pad;
    uint64_t timestamp;
    uint8_t  route[MAX_ROUTING_STEPS];
    uint8_t  data[EVENT_DATA_SIZE];
} Event;

// ============================================================================
// RESPONSE STRUCTURE (must match kernel)
// ============================================================================

typedef struct __attribute__((packed)) {
    uint64_t event_id;
    uint64_t workflow_id;
    uint32_t status;
    uint32_t error_code;
    uint64_t timestamp;
    void*    result_data;
    uint64_t result_size;
    uint8_t  completed_prefix;
    uint8_t  padding[7];
} Response;

// ============================================================================
// RING BUFFER STRUCTURE
// ============================================================================

typedef struct {
    uint64_t head __attribute__((aligned(64)));
    uint64_t tail __attribute__((aligned(64)));
    Event    events[256];
} EventRing;

typedef struct {
    uint64_t head __attribute__((aligned(64)));
    uint64_t tail __attribute__((aligned(64)));
    Response responses[256];
} ResponseRing;

// ============================================================================
// TAG STRUCTURE (for TagFS)
// ============================================================================

#define TAG_KEY_SIZE   32
#define TAG_VALUE_SIZE 64

typedef struct {
    char key[TAG_KEY_SIZE];
    char value[TAG_VALUE_SIZE];
} Tag;

// ============================================================================
// SYSCALL INTERFACE
// ============================================================================

// The ONE and ONLY syscall in BoxOS!
static inline uint64_t kernel_notify(uint64_t workflow_id, uint64_t flags) {
    uint64_t result;
    __asm__ volatile(
        "int $0x80"
        : "=a"(result)
        : "D"(workflow_id), "S"(flags)
        : "memory"
    );
    return result;
}

// ============================================================================
// CONSOLE API (High-level wrappers)
// ============================================================================

// Print string to console
void print(const char* str);

// Print string with color attribute
void print_attr(const char* str, uint8_t attr);

// Print single character
void putchar(char c);

// Read line from keyboard (returns static buffer, max 256 chars)
char* readline(void);

// Read single character (blocking)
char getchar(void);

// Clear screen
void clear(void);

// ============================================================================
// STRING UTILITIES
// ============================================================================

size_t strlen(const char* s);
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, size_t n);
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t n);
void* memset(void* s, int c, size_t n);
void* memcpy(void* dest, const void* src, size_t n);

// String parsing
char* strtok(char* str, const char* delim);
int starts_with(const char* str, const char* prefix);
int contains_char(const char* s, char c);

// Number conversion
int atoi(const char* str);
void itoa(int value, char* str);

// ============================================================================
// PROCESS CONTROL
// ============================================================================

// Yield CPU to other processes
static inline void yield(void) {
    kernel_notify(0, NOTIFY_YIELD);
}

// Exit process
static inline void exit(int code) {
    (void)code;  // BoxOS doesn't use exit codes yet
    kernel_notify(0, NOTIFY_EXIT);
    while(1) { __asm__ volatile("hlt"); }  // Should never reach
}

// Sleep for milliseconds (via timer event)
void sleep_ms(uint32_t ms);

#endif // ULIB_H
