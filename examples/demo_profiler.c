#include "ownedc.h"
#include <stdio.h>

int main(void) {
    printf("--- OwnedC Memory Profiler Demo ---\n");
    
    // Create some objects
    void* a = owner_malloc(1024);
    void* b = owner_malloc(256);
    void* c = owner_malloc(64);
    
    // Borrow b
    ownership_borrow(b);
    
    // Share c
    ownership_share(c);
    
    // Dump to JSON
    printf("Dumping memory state to 'memory_dump.json'...\n");
    ownership_dump_json("memory_dump.json");
    
    // Cleanup
    ownership_release(b);
    owner_free(a);
    owner_free(b);
    owner_free(c);
    
    printf("Done!\n");
    return 0;
}
