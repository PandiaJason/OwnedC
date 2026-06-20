#ifndef OWNEDC_ARRAY_H
#define OWNEDC_ARRAY_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Owned Array Structure
// Designed specifically to hold pointers to OTHER OwnedC objects.
// When the array is freed, it recursively frees all non-null pointers inside it.
typedef struct {
    void** data;
    size_t capacity;
} owned_array_t;

// Creates a new Owned Array with the given capacity.
owned_array_t* owned_array_new(size_t capacity);

// Sets the pointer at the given index. 
// Aborts dynamically if index is out of bounds.
void owned_array_set(owned_array_t* arr, size_t index, void* ptr);

// Gets the pointer at the given index.
// Aborts dynamically if index is out of bounds.
void* owned_array_get(owned_array_t* arr, size_t index);

// Frees the array AND calls owner_free() on every non-null pointer it contains.
void owned_array_free(owned_array_t* arr);

// Cleanup helper for the RAII macro
static inline void cleanup_owned_array_free(void* ptr) {
    owned_array_t** arr_ptr = (owned_array_t**)ptr;
    if (arr_ptr && *arr_ptr) {
        owned_array_free(*arr_ptr);
    }
}

// RAII Macro specifically for owned_array_t
#define OWNED_ARRAY __attribute__((cleanup(cleanup_owned_array_free)))

#ifdef __cplusplus
}
#endif

#endif // OWNEDC_ARRAY_H
