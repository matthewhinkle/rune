/**
 * Core runtime implementation.
 *
 * Implements:
 *   - Thread-local error stack with automatic location capture
 *   - Allocator stack management with push/pop semantics
 *   - Default malloc/realloc/free allocator
 *   - Type-safe memory allocation macros
 */

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "r.h"

/*
 * ================================================================================================
 * ERROR HANDLING
 * ================================================================================================
 */

// --------- Thread-local storage ---------

_Thread_local r_error_stack_t r_error_stack = {.depth = 0, .enabled = true};

// --------- API: Setup & Reporting ---------

bool R_(err_set)(r_error_code code, const char * message, const char * file, int line, const char * func) {
    if (!r_error_stack.enabled || r_error_stack.depth >= R_ERROR_STACK_MAX) {
        return false;
    }

    // Create immutable error context using compound literal
    // (all fields are const, preventing any accidental modifications)
    const r_error_ctx new_error = {
        .code = code,
        .message = message && message[0] != NULLTERM ? message : r_error_message(code),
        .file = file,
        .line = line,
        .func = func,
    };

    // Copy into stack (safe because memcpy works with const-qualified destinations)
    memcpy(&r_error_stack.stack[r_error_stack.depth++], &new_error, sizeof(r_error_ctx));
    return true;
}

void R_(err_print)(FILE * stream) {
    if (!R_(err_has)()) {
        fprintf(stream, "No error\n");
        return;
    }

    const r_error_ctx * err = R_(err_get)();
    fprintf(stream, "Error [%d]: %s\n", err->code, err->message);
    fprintf(stream, "  at %s:%d in %s()\n", err->file, err->line, err->func);
}

void R_(err_print_stack)(FILE * stream) {
    if (!R_(err_has)()) {
        fprintf(stream, "No errors\n");
        return;
    }

    fprintf(stream, "Error stack trace (depth=%d):\n", r_error_stack.depth);
    for (int i = r_error_stack.depth - 1; i >= 0; i--) {
        const r_error_ctx * err = &r_error_stack.stack[i];
        fprintf(stream, "  [%d] %s (%d) at %s:%d in %s()\n", i, err->message, err->code, err->file, err->line, err->func);
    }
}

// --------- Message lookup ---------

const char * r_error_message(r_error_code code) {
    switch (code) {
    case R_ERR_OK:
        return "No error";

    // Memory errors
    case R_ERR_ALLOC_FAILED:
        return "Memory allocation failed";
    case R_ERR_OUT_OF_MEMORY:
        return "Out of memory";
    case R_ERR_ALLOCATOR_STACK_OVERFLOW:
        return "Allocator stack overflow";
    case R_ERR_ALLOCATOR_STACK_UNDERFLOW:
        return "Allocator stack underflow";

    // Validation errors
    case R_ERR_NULL_POINTER:
        return "Null pointer argument";
    case R_ERR_INVALID_ARGUMENT:
        return "Invalid argument";
    case R_ERR_INVALID_LENGTH:
        return "Invalid length";
    case R_ERR_EMPTY_INPUT:
        return "Empty input";

    // Overflow errors
    case R_ERR_OVERFLOW:
        return "Numeric overflow";
    case R_ERR_BUFFER_OVERFLOW:
        return "Buffer overflow";
    case R_ERR_LENGTH_EXCEEDED:
        return "Maximum length exceeded";
    case R_ERR_CAPACITY_EXCEEDED:
        return "Capacity exceeded";

    // Not found errors
    case R_ERR_NOT_FOUND:
        return "Item not found";
    case R_ERR_PATTERN_NOT_FOUND:
        return "Pattern not found in string";
    case R_ERR_INDEX_OUT_OF_BOUNDS:
        return "Index out of bounds";

    // Collection errors
    case R_ERR_QUEUE_FULL:
        return "Queue is full";
    case R_ERR_QUEUE_EMPTY:
        return "Queue is empty";
    case R_ERR_LIST_EMPTY:
        return "List is empty";

    // Format errors
    case R_ERR_FORMAT_FAILED:
        return "Format operation failed";
    case R_ERR_PARSE_FAILED:
        return "Parse operation failed";

    // String errors
    case R_ERR_STRING_TOO_LONG:
        return "String exceeds maximum length";
    case R_ERR_INVALID_UTF8:
        return "Invalid UTF-8 sequence";

    default:
        return "Unknown error";
    }
}

// --------- API: Inspection ---------

const r_error_ctx * R_(err_get)(void) {
    if (r_error_stack.depth == 0) {
        return nullptr;
    }
    return &r_error_stack.stack[r_error_stack.depth - 1];
}

r_error_code R_(err_code)(void) {
    const r_error_ctx * err = R_(err_get)();
    return err ? err->code : R_ERR_OK;
}

const char * R_(err_msg)(void) {
    const r_error_ctx * err = R_(err_get)();
    return err ? err->message : "No error";
}

bool R_(err_has)(void) {
    return r_error_stack.depth > 0;
}

int R_(err_depth)(void) {
    return r_error_stack.depth;
}

const r_error_ctx * R_(err_at)(int index) {
    if (index < 0 || index >= r_error_stack.depth) {
        return nullptr;
    }
    return &r_error_stack.stack[index];
}

// --------- API: Management ---------

void R_(err_pop)(void) {
    if (r_error_stack.depth > 0) {
        r_error_stack.depth--;
    }
}

void R_(err_clear)(void) {
    r_error_stack.depth = 0;
}

void R_(err_enable)(bool enabled) {
    r_error_stack.enabled = enabled;
}

bool R_(err_is_enabled)(void) {
    return r_error_stack.enabled;
}

/*
 * ================================================================================================
 * ALLOCATOR IMPLEMENTATION
 * ================================================================================================
 */

// --------- Thread-local storage ---------

typedef struct {
    allocator stack[mem_alloc_STACK_MAX];
    int depth;
} mem_alloc_stack_t;

static _Thread_local mem_alloc_stack_t mem_alloc_stack = {.depth = 0};

// --------- Default allocator ---------

// ReSharper disable CppParameterMayBeConstPtrOrRef - match allocator struct function pointers
static void * r_default_alloc(void * ctx, size_t size) {
    (void)ctx;
    void * ptr = malloc(size);
    if (ptr == nullptr) {
        err_set(R_ERR_OUT_OF_MEMORY, nullptr);
    }
    assert(ptr != nullptr);
    return ptr;
}

static void * r_default_realloc(void * ctx, void * ptr, size_t old_size, size_t new_size) {
    (void)ctx;
    (void)old_size;
    void * new_ptr = realloc(ptr, new_size);
    if (new_ptr == nullptr) {
        err_set(R_ERR_OUT_OF_MEMORY, nullptr);
    }
    assert(new_ptr != nullptr);
    return new_ptr;
}

static void r_default_free(void * ctx, void * ptr, size_t size) {
    (void)ctx;
    (void)size;
    free(ptr);
}
// ReSharper restore CppParameterMayBeConstPtrOrRef

const allocator r_default_allocator = {
    .alloc = r_default_alloc,
    .realloc = r_default_realloc,
    .free = r_default_free,
    .ctx = nullptr,
};

// --------- API: Stack management ---------

extern void alloc_push(allocator a) {
    if (mem_alloc_stack.depth >= mem_alloc_STACK_MAX) {
        err_set(R_ERR_ALLOCATOR_STACK_OVERFLOW, nullptr);
    }
    assert(mem_alloc_stack.depth < mem_alloc_STACK_MAX && "Allocator stack overflow");
    mem_alloc_stack.stack[mem_alloc_stack.depth++] = a;
}

extern void alloc_pop(void) {
    if (mem_alloc_stack.depth == 0) {
        err_set(R_ERR_ALLOCATOR_STACK_UNDERFLOW, nullptr);
    }
    assert(mem_alloc_stack.depth > 0 && "Allocator stack underflow");
    mem_alloc_stack.depth--;
}

extern allocator alloc_current(void) {
    if (mem_alloc_stack.depth == 0) {
        return r_default_allocator;
    }
    return mem_alloc_stack.stack[mem_alloc_stack.depth - 1];
}

// --------- API: Memory operations ---------

extern void * mem_alloc(size_t size) {
    const allocator a = alloc_current();
    return a.alloc(a.ctx, size);
}

extern void * mem_alloc_zero(size_t size) {
    const allocator a = alloc_current();
    void * ptr = a.alloc(a.ctx, size);
    memset(ptr, 0, size);
    return ptr;
}

extern void * mem_realloc(void * ptr, size_t old_size, size_t new_size) {
    const allocator a = alloc_current();
    return a.realloc(a.ctx, ptr, old_size, new_size);
}

extern void mem_free(void * ptr, size_t size) {
    if (ptr != nullptr) {
        const allocator a = alloc_current();
        a.free(a.ctx, ptr, size);
    }
}
