#ifndef OWNEDC_COLLECTIONS_H
#define OWNEDC_COLLECTIONS_H

#include "ownedc.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void** data;
    size_t length;
    size_t capacity;
} safe_vector_t;

/* Create a new safe vector */
safe_vector_t* safe_vector_create(size_t initial_capacity);

/* Push an item to the end of the vector */
void safe_vector_push(safe_vector_t* vec, void* item);

/* Get an item at the specified index. Aborts if out of bounds. */
void* safe_vector_get(safe_vector_t* vec, size_t index);

/* Free the vector and its internal buffer.
 * Note: It does not free the individual items stored in the vector. 
 * If the items are owned, they should be freed separately. */
void safe_vector_free(safe_vector_t* vec);

/* Get the current length of the vector */
static inline size_t safe_vector_length(safe_vector_t* vec) {
    return vec ? vec->length : 0;
}

/* Remove an item at the specified index. Aborts if out of bounds. */
void safe_vector_remove(safe_vector_t* vec, size_t index);

#ifdef __cplusplus
}
#endif

#endif /* OWNEDC_COLLECTIONS_H */
