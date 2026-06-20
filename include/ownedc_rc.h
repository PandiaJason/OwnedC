#ifndef OWNEDC_RC_H
#define OWNEDC_RC_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Owned Reference Count Structure
// Wraps a raw pointer and keeps track of how many references point to it.
typedef struct {
    void* ptr;
    size_t ref_count;
} owned_rc_t;

// Creates a new Reference Counted wrapper around a dynamically allocated pointer.
// The pointer MUST be allocated using owner_malloc (or be NULL).
// The initial reference count is set to 1.
owned_rc_t* owned_rc_new(void* ptr);

// Increments the reference count and returns the wrapper.
owned_rc_t* owned_rc_clone(owned_rc_t* rc);

// Decrements the reference count.
// If the count reaches 0, both the internal pointer and the wrapper are completely freed.
void owned_rc_drop(owned_rc_t* rc);

// Retrieves the inner pointer for use.
void* owned_rc_get(owned_rc_t* rc);

// Cleanup helper for the RAII macro
static inline void cleanup_owned_rc_drop(void* ptr) {
    owned_rc_t** rc_ptr = (owned_rc_t**)ptr;
    if (rc_ptr && *rc_ptr) {
        owned_rc_drop(*rc_ptr);
    }
}

// RAII Macro specifically for owned_rc_t
// When this variable leaves scope, it automatically decrements the refcount.
#define OWNED_RC __attribute__((cleanup(cleanup_owned_rc_drop)))

#ifdef __cplusplus
}
#endif

#endif // OWNEDC_RC_H
