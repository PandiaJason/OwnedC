#include "ownedc.h"
#include "ownedc_collections.h"
#include "ownedc_region.h"
#include <stdio.h>
#include <pthread.h>

void* thread_worker(void* arg) {
    void* shared_data = arg;
    printf("[Worker] Borrowing shared data...\n");
    
    // We can borrow it because the main thread called ownership_share
    ownership_borrow(shared_data);
    
    printf("[Worker] Accessing shared data at %p\n", shared_data);
    
    ownership_release(shared_data);
    return NULL;
}

int main(void) {
    printf("=== OwnedC Full Showcase ===\n\n");

    printf("1. RAII Memory Management\n");
    {
        OWNED void* buffer = owner_malloc(1024);
        printf("   Allocated 1KB buffer at %p. It will automatically free when block ends.\n", buffer);
    } // owner_free is automatically called here
    printf("   Buffer freed automatically.\n\n");

    printf("2. Safe Collections (Bounds Checking)\n");
    OWNED safe_vector_t* vec = safe_vector_create(5);
    int a = 42;
    safe_vector_push(vec, &a);
    int* val = (int*)safe_vector_get(vec, 0);
    printf("   Stored and retrieved value from safe_vector: %d\n\n", *val);

    printf("3. High-Performance Arenas (Regions)\n");
    safe_region_t* region = safe_region_create(4096);
    void* entity1 = safe_region_alloc(region, 64);
    void* entity2 = safe_region_alloc(region, 128);
    printf("   Allocated entity1 at %p and entity2 at %p within Region Arena.\n", entity1, entity2);
    safe_region_free(region);
    printf("   Freed entire region at once.\n\n");

    printf("4. Thread Safety & Ownership Sharing\n");
    OWNED void* cross_thread_data = owner_malloc(256);
    
    // Explicitly share it to prevent a Thread Ownership Violation crash
    ownership_share(cross_thread_data);
    
    pthread_t t;
    pthread_create(&t, NULL, thread_worker, cross_thread_data);
    pthread_join(t, NULL);

    printf("\n=== Showcase Completed Successfully ===\n");
    ownership_stats();

    return 0;
}
