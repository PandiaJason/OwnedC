#include "ownedc_future.h"
#include "ownedc_env.h"
#ifndef OWNEDC_NO_STDLIB
#include <pthread.h>
#endif

struct owned_future {
    owned_task_fn task;
    void* arg;
    void* result;
    int is_ready;
#ifndef OWNEDC_NO_STDLIB
    pthread_t thread;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
#endif
};

#ifndef OWNEDC_NO_STDLIB
static void* future_thread_worker(void* f_arg) {
    owned_future_t* future = (owned_future_t*)f_arg;
    
    // Claim ownership of the argument for the background thread
    if (future->arg) {
        ownership_claim(future->arg);
    }
    
    void* res = future->task(future->arg);
    
    // In a real framework we'd need to transfer ownership of res to the future's context.
    // For now we just store it.
    
    pthread_mutex_lock(&future->mutex);
    future->result = res;
    future->is_ready = 1;
    pthread_cond_signal(&future->cond);
    pthread_mutex_unlock(&future->mutex);
    
    return NULL;
}
#endif

owned_future_t* owned_future_spawn(owned_task_fn task, void* arg) {
    owned_future_t* future = (owned_future_t*)owner_malloc(sizeof(owned_future_t));
    if (!future) return NULL;
    
    future->task = task;
    future->arg = arg;
    future->result = NULL;
    future->is_ready = 0;
    
#ifndef OWNEDC_NO_STDLIB
    pthread_mutex_init(&future->mutex, NULL);
    pthread_cond_init(&future->cond, NULL);
    pthread_create(&future->thread, NULL, future_thread_worker, future);
#else
    // Synchronous execution for bare metal
    future->result = task(arg);
    future->is_ready = 1;
#endif
    
    return future;
}

void* owned_future_await(owned_future_t* future) {
    if (!future) return NULL;
    
#ifndef OWNEDC_NO_STDLIB
    pthread_mutex_lock(&future->mutex);
    while (!future->is_ready) {
        pthread_cond_wait(&future->cond, &future->mutex);
    }
    void* res = future->result;
    pthread_mutex_unlock(&future->mutex);
    
    pthread_join(future->thread, NULL);
    pthread_mutex_destroy(&future->mutex);
    pthread_cond_destroy(&future->cond);
#else
    void* res = future->result;
#endif
    
    owner_free(future);
    
    // Ownership of 'res' is transferred to caller
    if (res) {
        ownership_claim(res);
    }
    
    return res;
}
