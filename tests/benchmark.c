#include "ownedc.h"
#include "ownedc_region.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#define NUM_ALLOCS 500000
#define NUM_THREADS 8
#define ALLOC_SIZE 32

double get_time_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

// Global arrays for threads
void* thread_ptrs[NUM_THREADS][NUM_ALLOCS / NUM_THREADS];

void* thread_raw_malloc(void* arg) {
    long tid = (long)arg;
    int allocs_per_thread = NUM_ALLOCS / NUM_THREADS;
    for (int i = 0; i < allocs_per_thread; i++) {
        thread_ptrs[tid][i] = malloc(ALLOC_SIZE);
    }
    for (int i = 0; i < allocs_per_thread; i++) {
        free(thread_ptrs[tid][i]);
    }
    return NULL;
}

void* thread_owner_malloc(void* arg) {
    long tid = (long)arg;
    int allocs_per_thread = NUM_ALLOCS / NUM_THREADS;
    for (int i = 0; i < allocs_per_thread; i++) {
        thread_ptrs[tid][i] = owner_malloc(ALLOC_SIZE);
    }
    for (int i = 0; i < allocs_per_thread; i++) {
        owner_free(thread_ptrs[tid][i]);
    }
    return NULL;
}

int main(void) {
    printf("--- OwnedC Benchmark ---\n");
    printf("Total Allocations: %d\n", NUM_ALLOCS);
    printf("Allocation Size:   %d bytes\n\n", ALLOC_SIZE);
    
    // 1. Single-threaded Raw malloc/free
    double start = get_time_sec();
    void** ptrs = malloc(NUM_ALLOCS * sizeof(void*));
    for (int i = 0; i < NUM_ALLOCS; i++) {
        ptrs[i] = malloc(ALLOC_SIZE);
    }
    for (int i = 0; i < NUM_ALLOCS; i++) {
        free(ptrs[i]);
    }
    double raw_time = get_time_sec() - start;
    printf("[Single-Thread] Raw malloc/free:   %.6f seconds\n", raw_time);
    
    // 2. Single-threaded owner_malloc/free
    start = get_time_sec();
    for (int i = 0; i < NUM_ALLOCS; i++) {
        ptrs[i] = owner_malloc(ALLOC_SIZE);
    }
    for (int i = 0; i < NUM_ALLOCS; i++) {
        owner_free(ptrs[i]);
    }
    double owned_time = get_time_sec() - start;
    printf("[Single-Thread] owner_malloc/free: %.6f seconds (%.2fx overhead)\n", owned_time, owned_time / raw_time);
    
    // 3. Single-threaded safe_region_alloc
    start = get_time_sec();
    safe_region_t* region = safe_region_create(NUM_ALLOCS * ALLOC_SIZE + 4096);
    for (int i = 0; i < NUM_ALLOCS; i++) {
        ptrs[i] = safe_region_alloc(region, ALLOC_SIZE);
    }
    safe_region_free(region);
    double region_time = get_time_sec() - start;
    printf("[Single-Thread] safe_region_alloc: %.6f seconds (%.2fx faster than raw)\n", region_time, raw_time / region_time);
    free(ptrs);
    
    printf("\n--- Multithreaded Contention Benchmark (%d Threads) ---\n", NUM_THREADS);
    
    // 4. Multi-threaded Raw malloc/free
    pthread_t threads[NUM_THREADS];
    start = get_time_sec();
    for (long i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, thread_raw_malloc, (void*)i);
    }
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    double mt_raw_time = get_time_sec() - start;
    printf("[Multi-Thread]  Raw malloc/free:   %.6f seconds\n", mt_raw_time);
    
    // 5. Multi-threaded owner_malloc/free
    start = get_time_sec();
    for (long i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, thread_owner_malloc, (void*)i);
    }
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    double mt_owned_time = get_time_sec() - start;
    printf("[Multi-Thread]  owner_malloc/free: %.6f seconds (%.2fx overhead under contention)\n", mt_owned_time, mt_owned_time / mt_raw_time);
    
    return 0;
}
