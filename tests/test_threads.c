#include "ownedc.h"
#include <stdio.h>
#include <pthread.h>
#include <unistd.h> // Ensure unistd.h is included for usleep/sleep depending on needs, but we don't strictly need it

void* thread_func(void* arg) {
    void* ptr = owner_malloc(50);
    ownership_inspect(ptr);
    owner_free(ptr);
    return NULL;
}

int main(void) {
    printf("Running concurrent thread allocation test...\n");
    pthread_t t1, t2;

    pthread_create(&t1, NULL, thread_func, NULL);
    pthread_create(&t2, NULL, thread_func, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    return 0;
}
