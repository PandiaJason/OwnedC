#include "ownedc.h"
#include "ownedc_future.h"
#include <stdio.h>

#ifndef OWNEDC_NO_STDLIB
#include <unistd.h>
#endif

void* my_async_task(void* arg) {
    int input = *(int*)arg;
    owner_free(arg); // Free the input argument (we took ownership)
    
#ifndef OWNEDC_NO_STDLIB
    printf("Task starting work in background...\n");
    sleep(1);
#endif
    
    // Allocate the result
    int* result = (int*)owner_malloc(sizeof(int));
    *result = input * 2;
    
#ifndef OWNEDC_NO_STDLIB
    printf("Task finished!\n");
#endif
    
    return result; // Transfer ownership of result to future
}

int main(void) {
    printf("--- OwnedC Futures Demo ---\n");
    
    int* arg = (int*)owner_malloc(sizeof(int));
    *arg = 21;
    
    // Spawn the future
    owned_future_t* future = owned_future_spawn(my_async_task, arg);
    
#ifndef OWNEDC_NO_STDLIB
    printf("Main thread is free to do other work!\n");
#endif
    
    // Await the result
    int* result = (int*)owned_future_await(future);
    
    printf("Result received from future: %d\n", *result);
    
    owner_free(result);
    
    printf("Demo finished successfully.\n");
    return 0;
}
