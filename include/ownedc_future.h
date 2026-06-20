#ifndef OWNEDC_FUTURE_H
#define OWNEDC_FUTURE_H

#include "ownedc.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* (*owned_task_fn)(void* arg);

typedef struct owned_future owned_future_t;

/**
 * @brief Spawn an asynchronous task that computes an owned value.
 * 
 * In standard mode, this spawns a background thread.
 * In NO_STDLIB mode, it executes synchronously.
 */
owned_future_t* owned_future_spawn(owned_task_fn task, void* arg);

/**
 * @brief Await the completion of the future and retrieve the result.
 * 
 * Transfers ownership of the result to the caller.
 * The caller must owner_free the result.
 * Frees the future object automatically after awaiting.
 */
void* owned_future_await(owned_future_t* future);

#ifdef __cplusplus
}
#endif

#endif // OWNEDC_FUTURE_H
