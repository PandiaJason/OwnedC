#include "ownedc_mutex.h"
#include "ownedc.h"
#include <stdio.h>

safe_mutex_t* safe_mutex_new(pthread_mutex_t* raw_mutex) {
    if (!raw_mutex) return NULL;
    
    safe_mutex_t* sm = (safe_mutex_t*)owner_malloc(sizeof(safe_mutex_t));
    if (!sm) return NULL;
    
    sm->raw_mutex = raw_mutex;
    sm->is_locked = false;
    
    return sm;
}

safe_mutex_t* safe_mutex_lock(safe_mutex_t* sm) {
    if (!sm) return NULL;
    
    // Actually lock the underlying mutex
    pthread_mutex_lock(sm->raw_mutex);
    sm->is_locked = true;
    
    return sm;
}

void safe_mutex_unlock(safe_mutex_t* sm) {
    if (!sm) return;
    
    if (sm->is_locked) {
        pthread_mutex_unlock(sm->raw_mutex);
        sm->is_locked = false;
    }
}

void safe_mutex_free(safe_mutex_t* sm) {
    if (!sm) return;
    
    // We do NOT destroy the pthread_mutex_t itself, as it might be shared globally.
    // We just free the RAII wrapper.
    owner_free(sm);
}
