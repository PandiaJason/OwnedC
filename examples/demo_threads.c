#include "ownedc.h"
#include <stdio.h>
#include <pthread.h>

void* thread_worker(void* arg) {
    void* shared_data = arg;
    printf("[Worker Thread] Accessing shared data at %p...\n", shared_data);
    
    // Safely borrow the data that the main thread shared
    ownership_borrow(shared_data);
    printf("[Worker Thread] Borrow successful! Processing...\n");
    ownership_release(shared_data);
    
    return NULL;
}

int main() {
    printf("--- OwnedC Thread Ownership & Safety Demo ---\n");
    
    // Allocated on the main thread
    OWNED void* data = owner_malloc(128);
    printf("[Main Thread] Allocated data at %p\n", data);
    
    // If we didn't call ownership_share, the worker thread would crash
    // with a Thread Ownership Violation.
    printf("[Main Thread] Sharing ownership of data...\n");
    ownership_share(data);
    
    pthread_t t;
    pthread_create(&t, NULL, thread_worker, data);
    pthread_join(t, NULL);
    
    printf("[Main Thread] Done. Auto-freeing data.\n");
    
    return 0;
}
