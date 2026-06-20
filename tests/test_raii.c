#include "ownedc.h"
#include <stdio.h>

void do_work(void) {
    // This pointer will be automatically freed when the function returns
    OWNED void* ptr = owner_malloc(128);
    printf("Allocated RAII buffer at %p\n", ptr);
    // Notice no owner_free(ptr)
}

int main(void) {
    printf("Running RAII (GCC __attribute__((cleanup))) test...\n");
    
    do_work();

    // If RAII works, there should be 0 active allocations here.
    ownership_stats();

    return 0; // If it didn't work, a leak will be detected at exit.
}
