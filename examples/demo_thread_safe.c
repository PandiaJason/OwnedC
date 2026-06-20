#include "ownedc.h"
#include "ownedc_thread.h"
#include <stdio.h>
#include <unistd.h>

void* background_worker(void* arg) {
    int id = *(int*)arg;
    printf("[Worker %d] Starting work...\n", id);
    sleep(1); // Simulate work
    printf("[Worker %d] Finished work.\n", id);
    return NULL;
}

void test_safe_threads(void) {
    int worker_id = 1;
    
    // 1. Spawn a thread. It defaults to THREAD_CLEANUP_JOIN.
    OWNED_THREAD safe_thread_t* thread1 = safe_thread_spawn(background_worker, &worker_id);
    
    printf("[Main] Thread spawned successfully. I'm returning early!\n");
    // As we return, the OWNED_THREAD macro automatically calls pthread_join()
    // preventing the main thread from exiting before the worker is done!
    return;
}

int main(void) {
    printf("--- OwnedC Safe Threads Demo ---\n");
    test_safe_threads();
    printf("[Main] All workers cleanly joined.\n");
    return 0;
}
