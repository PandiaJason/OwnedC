#include "ownedc.h"

void intentional_leak() {
    void* leaked_ptr = owner_malloc(100);
    // Forgot to free
}

void intentional_double_free() {
    void* dfree_ptr = owner_malloc(50);
    owner_free(dfree_ptr);
    owner_free(dfree_ptr);
}

void intentional_use_after_free() {
    void* uaf_ptr = owner_malloc(30);
    owner_free(uaf_ptr);
    ownership_borrow(uaf_ptr);
}

void safe_function() {
    void* safe_ptr = owner_malloc(10);
    owner_free(safe_ptr);
}
