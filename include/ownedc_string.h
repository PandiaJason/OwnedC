#ifndef OWNEDC_STRING_H
#define OWNEDC_STRING_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Safe String Structure
typedef struct {
    char* data;
    size_t length;
    size_t capacity;
} safe_string_t;

// Create a new safe string. If initial_cstr is NULL, an empty string is created.
// The returned safe_string_t is managed by OwnedC and should be used with the OWNED macro.
safe_string_t* safe_string_new(const char* initial_cstr);

// Returns the underlying C-string. Guaranteed to be null-terminated.
const char* safe_string_cstr(safe_string_t* str);

// Appends a raw C-string to the safe string. Automatically resizes if needed.
void safe_string_append(safe_string_t* str, const char* suffix);

// Appends another safe string.
void safe_string_append_safe(safe_string_t* str, safe_string_t* suffix);

// Appends a single character.
void safe_string_push_char(safe_string_t* str, char c);

// Returns the character at the specified index. Aborts dynamically if out of bounds.
char safe_string_char_at(safe_string_t* str, size_t index);

// Truncates the string to a new length. Aborts if new_length > current length.
void safe_string_truncate(safe_string_t* str, size_t new_length);

// Free the safe string explicitly. (Usually not needed if used with OWNED_STRING)
void safe_string_free(safe_string_t* str);

// Cleanup helper for the RAII macro
static inline void cleanup_safe_string_free(void* ptr) {
    safe_string_t** str_ptr = (safe_string_t**)ptr;
    if (str_ptr && *str_ptr) {
        safe_string_free(*str_ptr);
    }
}

// RAII Macro specifically for safe_string_t
#define OWNED_STRING __attribute__((cleanup(cleanup_safe_string_free)))

#ifdef __cplusplus
}
#endif

#endif // OWNEDC_STRING_H
