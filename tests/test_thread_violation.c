#include "ownedc.h"
#include <stdio.h>
#include <pthread.h>

void* ptr = NULL;

void* thread_func(void* arg) { (void)arg;
    // Thread B tries to free Thread A's memory without sharing
    owner_free(ptr);
    return NULL;
}

int main(void) {
    printf("Running thread ownership violation test...\n");
    
    // Thread A allocates
    ptr = owner_malloc(100);

    pthread_t t;
    pthread_create(&t, NULL, thread_func, NULL);
    pthread_join(t, NULL);

    return 0;
}
