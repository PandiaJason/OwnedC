#ifndef OWNEDC_MUTEX_H
#define OWNEDC_MUTEX_H

#include <pthread.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Safe Mutex wrapper
typedef struct {
    pthread_mutex_t* raw_mutex;
    bool is_locked;
} safe_mutex_t;

// Creates a new RAII wrapper for an existing pthread_mutex_t
// NOTE: This does NOT initialize the mutex, it only wraps it for RAII locking.
safe_mutex_t* safe_mutex_new(pthread_mutex_t* mutex);

// Locks the mutex and returns the wrapper.
// This is what you should assign to OWNED_LOCK
safe_mutex_t* safe_mutex_lock(safe_mutex_t* sm);

// Unlocks the mutex manually (if desired).
// The macro knows not to unlock it twice.
void safe_mutex_unlock(safe_mutex_t* sm);

// Frees the wrapper itself (does NOT destroy the underlying pthread_mutex_t)
void safe_mutex_free(safe_mutex_t* sm);

// Cleanup helper for the RAII macro
static inline void cleanup_safe_mutex_unlock(void* ptr) {
    safe_mutex_t** sm_ptr = (safe_mutex_t**)ptr;
    if (sm_ptr && *sm_ptr) {
        // Unlock if it is currently locked
        safe_mutex_unlock(*sm_ptr);
        // Free the wrapper
        safe_mutex_free(*sm_ptr);
    }
}

// RAII Macro specifically for mutex locking.
// Automatically unlocks the mutex when dropping out of scope.
#define OWNED_LOCK __attribute__((cleanup(cleanup_safe_mutex_unlock)))

#ifdef __cplusplus
}
#endif

#endif // OWNEDC_MUTEX_H
