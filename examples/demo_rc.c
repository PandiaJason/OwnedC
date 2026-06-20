#include "ownedc.h"
#include "ownedc_rc.h"
#include <stdio.h>
#include <string.h>

void use_shared_data(owned_rc_t* shared) {
    // 3. Clone the reference for local usage.
    // The macro will automatically call owned_rc_drop when returning!
    OWNED_RC owned_rc_t* local_ref = owned_rc_clone(shared);
    
    printf("    [Function] Refcount is now %zu. Data: %s\n", local_ref->ref_count, (char*)owned_rc_get(local_ref));
    printf("    [Function] Returning. Local reference will drop.\n");
}

void demo_shared_ownership(void) {
    printf("--- OwnedC Reference Counting Demo ---\n");
    
    // 1. Allocate some data
    char* data = owner_malloc(50);
    strcpy(data, "I am shared memory!");
    
    // 2. Wrap it in a Reference Count. It starts with 1 ref.
    OWNED_RC owned_rc_t* main_ref = owned_rc_new(data);
    printf("[Main] Created shared data. Refcount is %zu.\n", main_ref->ref_count);
    
    // 3. Pass it to a function
    use_shared_data(main_ref);
    
    // 4. Check refcount after the function returns
    printf("[Main] Function returned. Refcount is back to %zu.\n", main_ref->ref_count);
    
    printf("[Main] Returning from main block. Final reference will drop and free memory!\n");
}

int main(void) {
    demo_shared_ownership();
    
    return 0;
}
