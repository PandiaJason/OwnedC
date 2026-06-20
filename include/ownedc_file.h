#ifndef OWNEDC_FILE_H
#define OWNEDC_FILE_H

#include <stdio.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Safe File wrapper ensuring guaranteed closure via RAII
typedef struct {
    FILE* handle;
    bool is_closed;
} safe_file_t;

// Safely opens a file and tracks it.
// Returns NULL if the file couldn't be opened.
safe_file_t* safe_fopen(const char* filename, const char* mode);

// Closes the file early (if desired).
// The macro will know not to close it again.
void safe_fclose(safe_file_t* file);

// Get the raw FILE* pointer for standard library operations (e.g., fread/fwrite)
FILE* safe_file_get(safe_file_t* file);

// Cleanup helper for the RAII macro
static inline void cleanup_safe_file(void* ptr) {
    safe_file_t** file_ptr = (safe_file_t**)ptr;
    if (file_ptr && *file_ptr) {
        safe_fclose(*file_ptr);
    }
}

// RAII Macro specifically for file I/O
// Ensures the file is automatically closed when dropping out of scope.
#define OWNED_FILE __attribute__((cleanup(cleanup_safe_file)))

#ifdef __cplusplus
}
#endif

#endif // OWNEDC_FILE_H
