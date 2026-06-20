#include "ownedc.h"
#include <stdio.h>

void do_work(void) {
    printf("[RAII Demo] Allocating buffer...\n");
    // Memory is automatically freed when the function exits
    OWNED void* buffer = owner_malloc(1024);
    
    printf("[RAII Demo] Processing data at %p...\n", buffer);
    // Work happens here...
    
    printf("[RAII Demo] Exiting function. Buffer will be automatically freed.\n");
}

int main(void) {
    printf("--- OwnedC RAII Auto-Cleanup Demo ---\n");
    do_work();
    
    // Prove there are no leaks
    ownership_stats();
    return 0;
}
