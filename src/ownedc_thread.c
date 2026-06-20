#include "ownedc_thread.h"
#include "ownedc.h"
#include "ownedc_env.h"


safe_thread_t* safe_thread_spawn(void *(*start_routine)(void *), void *arg) {
    safe_thread_t* thread = (safe_thread_t*)owner_malloc(sizeof(safe_thread_t));
    if (!thread) return NULL;
    
    thread->policy = THREAD_CLEANUP_JOIN;
    thread->is_active = true;
    
    if (pthread_create(&thread->handle, NULL, start_routine, arg) != 0) {
        owner_free(thread);
        return NULL;
    }
    
    return thread;
}

void safe_thread_set_policy(safe_thread_t* thread, thread_cleanup_policy_t policy) {
    if (thread) {
        thread->policy = policy;
    }
}

void safe_thread_cleanup(safe_thread_t* thread) {
    if (!thread) return;
    
    if (thread->is_active) {
        if (thread->policy == THREAD_CLEANUP_JOIN) {
            pthread_join(thread->handle, NULL);
        } else if (thread->policy == THREAD_CLEANUP_DETACH) {
            pthread_detach(thread->handle);
        }
        thread->is_active = false;
    }
    
    owner_free(thread);
}
