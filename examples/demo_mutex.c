#include "ownedc.h"
#include "ownedc_mutex.h"
#include <stdio.h>
#include <pthread.h>

pthread_mutex_t shared_mutex = PTHREAD_MUTEX_INITIALIZER;
int shared_counter = 0;

void process_critical_section(int task_id, bool simulate_error) {
    // 1. Create the RAII wrapper. This does NOT lock it yet.
    safe_mutex_t* sm = safe_mutex_new(&shared_mutex);
    
    // 2. Lock it using the RAII macro.
    // If we return early, the macro automatically unlocks it!
    OWNED_LOCK safe_mutex_t* lock = safe_mutex_lock(sm);
    
    printf("[Task %d] Entered critical section.\n", task_id);
    
    if (simulate_error) {
        printf("[Task %d] Encountered error! Returning early...\n", task_id);
        return; // DANGER: Standard C would deadlock here! OwnedC unlocks automatically.
    }
    
    shared_counter++;
    printf("[Task %d] Incremented counter to %d.\n", task_id, shared_counter);
    printf("[Task %d] Leaving normally.\n", task_id);
}

int main(void) {
    printf("--- OwnedC Safe Mutex Demo ---\n");
    
    printf("\nExecuting normal task:\n");
    process_critical_section(1, false);
    
    printf("\nExecuting task that throws an error and returns early:\n");
    process_critical_section(2, true);
    
    printf("\nExecuting another normal task to prove the mutex was unlocked:\n");
    process_critical_section(3, false);
    
    return 0;
}
