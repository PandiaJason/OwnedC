#include "ownedc.h"
#include "ownedc_rc.h"
#include <stdio.h>
#include <stdlib.h>

void simulate_memory_leak_cycle(void) {
    printf("Creating Node A and Node B...\n");
    // Allocate raw pointers
    int* dataA = (int*)owner_malloc(sizeof(int));
    int* dataB = (int*)owner_malloc(sizeof(int));
    *dataA = 1;
    *dataB = 2;
    
    // Wrap them in Reference Counting
    owned_rc_t* rcA = owned_rc_new(dataA);
    owned_rc_t* rcB = owned_rc_new(dataB);
    
    // Create a cycle! A -> B and B -> A
    // In standard C (or C++ std::shared_ptr without weak_ptr), this leaks memory forever!
    owned_rc_register_child(rcA, rcB);
    owned_rc_clone(rcB); // A holds a reference to B
    
    owned_rc_register_child(rcB, rcA);
    owned_rc_clone(rcA); // B holds a reference to A
    
    printf("RC Graph created. Dropping root references to A and B...\n");
    owned_rc_drop(rcA);
    owned_rc_drop(rcB);
    
    // Wait, since they reference each other, their ref_counts are 1. They are LEAKED.
    printf("Nodes dropped. But because of the cycle, they are isolated and leaked in RAM.\n");
}

int main(void) {
    printf("--- OwnedC GC Cycle Detection Demo ---\n");
    
    simulate_memory_leak_cycle();
    
    printf("\nRunning the OwnedC Garbage Collector...\n");
    int freed = owned_rc_collect_cycles();
    
    printf("Garbage Collector finished. Freed %d cyclic objects!\n", freed);
    
    if (freed == 2) {
        printf("SUCCESS: OwnedC successfully detected the isolated cycle and cleaned the memory.\n");
    } else {
        printf("FAILED: GC did not recover the expected 2 objects.\n");
    }
    
    return 0;
}
