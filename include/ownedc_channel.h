#ifndef OWNEDC_CHANNEL_H
#define OWNEDC_CHANNEL_H

#include "ownedc.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef OWNEDC_NO_STDLIB

typedef struct ownedc_channel ownedc_channel_t;

/**
 * @brief Create a new Multiple-Producer Single-Consumer (MPSC) channel.
 */
ownedc_channel_t* ownedc_channel_create(void);

/**
 * @brief Send an owned pointer through the channel.
 * Ownership is formally detached from the sending thread.
 */
void ownedc_channel_send(ownedc_channel_t* ch, void* ptr);

/**
 * @brief Receive an owned pointer from the channel.
 * Blocks until a message is available. Ownership is claimed by the receiver.
 */
void* ownedc_channel_recv(ownedc_channel_t* ch);

/**
 * @brief Free the channel. The channel should be empty, otherwise remaining items leak.
 */
void ownedc_channel_free(ownedc_channel_t* ch);

#endif // OWNEDC_NO_STDLIB

#ifdef __cplusplus
}
#endif

#endif // OWNEDC_CHANNEL_H
