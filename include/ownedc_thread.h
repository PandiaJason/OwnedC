#ifndef OWNEDC_THREAD_H
#define OWNEDC_THREAD_H

#include <pthread.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Options for thread cleanup behavior when dropping out of scope
typedef enum {
    THREAD_CLEANUP_JOIN,    // Block and wait for thread to finish (Default)
    THREAD_CLEANUP_DETACH   // Detach the thread and let it run in the background
} thread_cleanup_policy_t;

typedef struct {
    pthread_t handle;
    thread_cleanup_policy_t policy;
    bool is_active;
} safe_thread_t;

// Spawns a new thread executing `start_routine` with `arg`.
// Defaults to THREAD_CLEANUP_JOIN policy.
safe_thread_t* safe_thread_spawn(void *(*start_routine)(void *), void *arg);

// Changes the cleanup policy for the thread wrapper.
void safe_thread_set_policy(safe_thread_t* thread, thread_cleanup_policy_t policy);

// Manually clean up the thread early.
// The macro knows not to clean it up again.
void safe_thread_cleanup(safe_thread_t* thread);

// Cleanup helper for the RAII macro
static inline void cleanup_safe_thread(void* ptr) {
    safe_thread_t** thread_ptr = (safe_thread_t**)ptr;
    if (thread_ptr && *thread_ptr) {
        safe_thread_cleanup(*thread_ptr);
    }
}

// RAII Macro specifically for threads
// Ensures the thread is safely joined or detached when dropping out of scope.
#define OWNED_THREAD __attribute__((cleanup(cleanup_safe_thread)))

#ifdef __cplusplus
}
#endif

#endif // OWNEDC_THREAD_H
