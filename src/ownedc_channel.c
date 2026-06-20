#include "ownedc_channel.h"
#include "ownedc_env.h"

#ifndef OWNEDC_NO_STDLIB

#include <pthread.h>

// Simple linked list node for the unbounded queue
typedef struct channel_node {
    void* data;
    struct channel_node* next;
} channel_node_t;

struct ownedc_channel {
    channel_node_t* head;
    channel_node_t* tail;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
};

ownedc_channel_t* ownedc_channel_create(void) {
    ownedc_channel_t* ch = (ownedc_channel_t*)owner_malloc(sizeof(ownedc_channel_t));
    if (!ch) return NULL;
    
    ch->head = NULL;
    ch->tail = NULL;
    pthread_mutex_init(&ch->mutex, NULL);
    pthread_cond_init(&ch->cond, NULL);
    
    return ch;
}

void ownedc_channel_send(ownedc_channel_t* ch, void* ptr) {
    if (!ch || !ptr) return;
    
    // Detach ownership from the sending thread by sharing it temporarily
    // Realistically, the framework should have `ownership_detach()` but we can just
    // use the fact that `ownership_claim` in recv will re-assign the owner.
    // Wait, if it stays OWNED, `owner_free` on the receiver side would fail thread checks!
    // So we share it here so the thread check is bypassed, then the receiver claims it.
    ownership_share(ptr);
    
    channel_node_t* node = (channel_node_t*)owner_malloc(sizeof(channel_node_t));
    node->data = ptr;
    node->next = NULL;
    
    pthread_mutex_lock(&ch->mutex);
    if (ch->tail) {
        ch->tail->next = node;
        ch->tail = node;
    } else {
        ch->head = node;
        ch->tail = node;
    }
    pthread_cond_signal(&ch->cond);
    pthread_mutex_unlock(&ch->mutex);
}

void* ownedc_channel_recv(ownedc_channel_t* ch) {
    if (!ch) return NULL;
    
    pthread_mutex_lock(&ch->mutex);
    while (ch->head == NULL) {
        pthread_cond_wait(&ch->cond, &ch->mutex);
    }
    
    channel_node_t* node = ch->head;
    ch->head = node->next;
    if (ch->head == NULL) {
        ch->tail = NULL;
    }
    pthread_mutex_unlock(&ch->mutex);
    
    void* ptr = node->data;
    
    // Claim the internal node so we can safely free it
    ownership_claim(node);
    owner_free(node);
    
    // Receiver claims pure ownership
    if (ptr) {
        ownership_claim(ptr);
    }
    
    return ptr;
}

void ownedc_channel_free(ownedc_channel_t* ch) {
    if (!ch) return;
    
    pthread_mutex_destroy(&ch->mutex);
    pthread_cond_destroy(&ch->cond);
    
    // Any remaining nodes are leaked conceptually, but we free the linked list structure
    channel_node_t* curr = ch->head;
    while (curr) {
        channel_node_t* next = curr->next;
        owner_free(curr);
        curr = next;
    }
    
    owner_free(ch);
}

#endif // OWNEDC_NO_STDLIB
