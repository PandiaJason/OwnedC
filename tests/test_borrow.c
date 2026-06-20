#include "ownedc.h"
#include <stdio.h>

int main(void) {
    printf("Running borrow test...\n");
    void* ptr = owner_malloc(100);
    if (!ptr) return 1;

    // Borrow the pointer
    ownership_borrow(ptr);

    // This should fail because the pointer is currently borrowed
    owner_free(ptr);

    return 0;
}
