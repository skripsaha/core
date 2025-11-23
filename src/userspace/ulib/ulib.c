// ============================================================================
// BOXOS USER LIBRARY - Implementation
// ============================================================================

#include "ulib.h"

// ============================================================================
// INTERNAL: Ring buffer access
// ============================================================================

static EventRing* event_ring = (EventRing*)EVENT_RING_ADDR;
static ResponseRing* response_ring = (ResponseRing*)RESULT_RING_ADDR;

// Global event ID counter
static uint64_t next_event_id = 1;

// Static buffers
static char readline_buffer[256];
static char strtok_buffer[256];
static char* strtok_pos = NULL;

// ============================================================================
// INTERNAL: Event submission
// ============================================================================

static int push_event(Event* ev) {
    uint64_t tail = event_ring->tail;
    uint64_t head = event_ring->head;

    // Check if full
    if (tail - head >= 256) {
        return 0;  // Ring full
    }

    // Copy event to ring
    uint64_t idx = tail & 0xFF;
    Event* slot = &event_ring->events[idx];

    // Copy 256 bytes
    uint64_t* src = (uint64_t*)ev;
    uint64_t* dst = (uint64_t*)slot;
    for (int i = 0; i < 32; i++) {  // 256/8 = 32 qwords
        dst[i] = src[i];
    }

    // Memory barrier and update tail
    __asm__ volatile("mfence" ::: "memory");
    event_ring->tail = tail + 1;

    return 1;
}

static int pop_response(Response* resp) {
    uint64_t head = response_ring->head;
    uint64_t tail = response_ring->tail;

    // Check if empty
    if (head >= tail) {
        return 0;
    }

    // Copy response from ring
    uint64_t idx = head & 0xFF;
    Response* slot = &response_ring->responses[idx];

    // Copy response structure
    uint64_t* src = (uint64_t*)slot;
    uint64_t* dst = (uint64_t*)resp;
    for (int i = 0; i < sizeof(Response)/8; i++) {
        dst[i] = src[i];
    }

    // Update head
    __asm__ volatile("mfence" ::: "memory");
    response_ring->head = head + 1;

    return 1;
}

// ============================================================================
// INTERNAL: Simple event execution
// ============================================================================

// Execute a single event and wait for response
static int execute_event(uint32_t type, uint8_t deck_prefix,
                         void* payload, size_t payload_size,
                         Response* out_response) {
    // Build event
    Event ev;
    memset(&ev, 0, sizeof(ev));

    ev.id = next_event_id++;
    ev.user_id = 1;  // Default workflow ID
    ev.type = type;
    ev.timestamp = 0;  // Kernel fills this

    // Route: deck_prefix → 0 (execution)
    ev.route[0] = deck_prefix;
    ev.route[1] = 0;  // Execution deck

    // Copy payload
    if (payload && payload_size > 0) {
        if (payload_size > EVENT_DATA_SIZE) {
            payload_size = EVENT_DATA_SIZE;
        }
        memcpy(ev.data, payload, payload_size);
    }

    // Push event
    if (!push_event(&ev)) {
        return 0;  // Failed to push
    }

    // Notify kernel to process
    kernel_notify(1, NOTIFY_SUBMIT);

    // Wait for completion
    kernel_notify(1, NOTIFY_WAIT);

    // Get response
    if (out_response) {
        return pop_response(out_response);
    }

    // Drain response even if not needed
    Response dummy;
    pop_response(&dummy);
    return 1;
}

// ============================================================================
// CONSOLE API
// ============================================================================

void print(const char* str) {
    if (!str) return;

    size_t len = strlen(str);
    if (len == 0) return;
    if (len > EVENT_DATA_SIZE - 4) {
        len = EVENT_DATA_SIZE - 4;
    }

    // Build payload: [size:4][string:...]
    uint8_t payload[EVENT_DATA_SIZE];
    *(uint32_t*)payload = (uint32_t)len;
    memcpy(payload + 4, str, len);

    execute_event(EVENT_CONSOLE_WRITE, 3, payload, 4 + len, NULL);
}

void print_attr(const char* str, uint8_t attr) {
    if (!str) return;

    size_t len = strlen(str);
    if (len == 0) return;
    if (len > EVENT_DATA_SIZE - 5) {
        len = EVENT_DATA_SIZE - 5;
    }

    // Build payload: [attr:1][size:4][string:...]
    uint8_t payload[EVENT_DATA_SIZE];
    payload[0] = attr;
    *(uint32_t*)(payload + 1) = (uint32_t)len;
    memcpy(payload + 5, str, len);

    execute_event(EVENT_CONSOLE_WRITE_ATTR, 3, payload, 5 + len, NULL);
}

void putchar(char c) {
    char buf[2] = { c, '\0' };
    print(buf);
}

char* readline(void) {
    // Build payload: [max_size:4]
    uint32_t max_size = 256;

    Response resp;
    execute_event(EVENT_CONSOLE_READ_LINE, 3, &max_size, 4, &resp);

    // Copy result to static buffer
    if (resp.result_data && resp.status == 0) {
        char* result = (char*)resp.result_data;
        size_t len = strlen(result);
        if (len >= 256) len = 255;
        memcpy(readline_buffer, result, len);
        readline_buffer[len] = '\0';
        return readline_buffer;
    }

    readline_buffer[0] = '\0';
    return readline_buffer;
}

char getchar(void) {
    Response resp;
    execute_event(EVENT_CONSOLE_READ_CHAR, 3, NULL, 0, &resp);
    return (char)(uint64_t)resp.result_data;
}

void clear(void) {
    execute_event(EVENT_CONSOLE_CLEAR, 3, NULL, 0, NULL);
}

// ============================================================================
// STRING UTILITIES
// ============================================================================

size_t strlen(const char* s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return (unsigned char)*s1 - (unsigned char)*s2;
}

int strncmp(const char* s1, const char* s2, size_t n) {
    while (n && *s1 && *s1 == *s2) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return (unsigned char)*s1 - (unsigned char)*s2;
}

char* strcpy(char* dest, const char* src) {
    char* d = dest;
    while ((*d++ = *src++));
    return dest;
}

char* strncpy(char* dest, const char* src, size_t n) {
    char* d = dest;
    while (n && (*d++ = *src++)) n--;
    while (n--) *d++ = '\0';
    return dest;
}

void* memset(void* s, int c, size_t n) {
    unsigned char* p = s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}

void* memcpy(void* dest, const void* src, size_t n) {
    unsigned char* d = dest;
    const unsigned char* s = src;
    while (n--) *d++ = *s++;
    return dest;
}

char* strtok(char* str, const char* delim) {
    if (str) {
        // New string - copy to buffer
        strncpy(strtok_buffer, str, 255);
        strtok_buffer[255] = '\0';
        strtok_pos = strtok_buffer;
    }

    if (!strtok_pos || !*strtok_pos) {
        return NULL;
    }

    // Skip leading delimiters
    while (*strtok_pos && contains_char(delim, *strtok_pos)) {
        strtok_pos++;
    }

    if (!*strtok_pos) {
        return NULL;
    }

    // Find token start
    char* token_start = strtok_pos;

    // Find end of token
    while (*strtok_pos && !contains_char(delim, *strtok_pos)) {
        strtok_pos++;
    }

    // Null terminate
    if (*strtok_pos) {
        *strtok_pos++ = '\0';
    }

    return token_start;
}

int starts_with(const char* str, const char* prefix) {
    while (*prefix) {
        if (*str++ != *prefix++) {
            return 0;
        }
    }
    return 1;
}

int contains_char(const char* s, char c) {
    while (*s) {
        if (*s++ == c) return 1;
    }
    return 0;
}

int atoi(const char* str) {
    int result = 0;
    int sign = 1;

    // Skip whitespace
    while (*str == ' ' || *str == '\t') str++;

    // Handle sign
    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }

    // Convert digits
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }

    return sign * result;
}

void itoa(int value, char* str) {
    char* p = str;
    int negative = 0;

    if (value < 0) {
        negative = 1;
        value = -value;
    }

    // Generate digits in reverse
    do {
        *p++ = '0' + (value % 10);
        value /= 10;
    } while (value);

    if (negative) {
        *p++ = '-';
    }
    *p = '\0';

    // Reverse string
    char* start = str;
    char* end = p - 1;
    while (start < end) {
        char tmp = *start;
        *start++ = *end;
        *end-- = tmp;
    }
}

// ============================================================================
// PROCESS CONTROL
// ============================================================================

void sleep_ms(uint32_t ms) {
    // Build payload: [ms:8]
    uint64_t payload = ms;
    execute_event(EVENT_TIMER_SLEEP, 3, &payload, 8, NULL);
}
