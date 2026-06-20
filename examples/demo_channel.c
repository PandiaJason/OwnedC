#include "ownedc.h"
#include "ownedc_channel.h"
#include <stdio.h>

#ifndef OWNEDC_NO_STDLIB
#include <pthread.h>
#include <unistd.h>

void* producer_thread(void* arg) {
    ownedc_channel_t* ch = (ownedc_channel_t*)arg;
    
    for (int i = 0; i < 5; i++) {
        int* msg = (int*)owner_malloc(sizeof(int));
        *msg = i * 10;
        printf("[Producer] Sending message: %d\n", *msg);
        ownedc_channel_send(ch, msg);
        sleep(1);
    }
    
    // Send termination signal (NULL not allowed by channel_send due to my check, so send a special value)
    int* end_msg = (int*)owner_malloc(sizeof(int));
    *end_msg = -1;
    ownedc_channel_send(ch, end_msg);
    
    return NULL;
}

int main(void) {
    printf("--- OwnedC Channels Demo ---\n");
    
    ownedc_channel_t* ch = ownedc_channel_create();
    
    pthread_t prod;
    pthread_create(&prod, NULL, producer_thread, ch);
    
    while (1) {
        int* msg = (int*)ownedc_channel_recv(ch);
        int val = *msg;
        printf("[Consumer] Received message: %d\n", val);
        owner_free(msg); // Safely free since we claimed ownership
        
        if (val == -1) {
            break;
        }
    }
    
    pthread_join(prod, NULL);
    ownedc_channel_free(ch);
    
    printf("Demo finished successfully.\n");
    return 0;
}
#else
int main(void) {
    printf("Channels require threads (NO_STDLIB is active).\n");
    return 0;
}
#endif
