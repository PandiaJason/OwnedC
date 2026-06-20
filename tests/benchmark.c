#include "ownedc.h"
#include "ownedc_region.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NUM_ALLOCS 100000

double get_time_sec() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

int main() {
    printf("--- OwnedC Benchmark ---\n");
    printf("Allocations per test: %d\n\n", NUM_ALLOCS);
    
    // 1. Raw malloc/free
    double start = get_time_sec();
    void* ptrs[NUM_ALLOCS];
    for (int i = 0; i < NUM_ALLOCS; i++) {
        ptrs[i] = malloc(32);
    }
    for (int i = 0; i < NUM_ALLOCS; i++) {
        free(ptrs[i]);
    }
    double raw_time = get_time_sec() - start;
    printf("Raw malloc/free:      %f seconds\n", raw_time);
    
    // 2. owner_malloc/owner_free
    start = get_time_sec();
    for (int i = 0; i < NUM_ALLOCS; i++) {
        ptrs[i] = owner_malloc(32);
    }
    for (int i = 0; i < NUM_ALLOCS; i++) {
        owner_free(ptrs[i]);
    }
    double owned_time = get_time_sec() - start;
    printf("owner_malloc/free:    %f seconds (%.2fx overhead)\n", owned_time, owned_time / raw_time);
    
    // 3. safe_region_alloc
    start = get_time_sec();
    safe_region_t* region = safe_region_create(NUM_ALLOCS * 32 + 4096);
    for (int i = 0; i < NUM_ALLOCS; i++) {
        safe_region_alloc(region, 32);
    }
    safe_region_free(region);
    double region_time = get_time_sec() - start;
    printf("safe_region_alloc:    %f seconds (%.2fx compared to raw malloc)\n", region_time, region_time / raw_time);
    
    return 0;
}
