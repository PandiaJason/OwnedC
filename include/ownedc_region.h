#ifndef OWNEDC_REGION_H
#define OWNEDC_REGION_H

#include "ownedc.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct safe_region_chunk safe_region_chunk_t;

typedef struct {
    safe_region_chunk_t* current_chunk;
    size_t chunk_size;
} safe_region_t;

/* Create a new safe region with a specified chunk capacity */
safe_region_t* safe_region_create(size_t chunk_size);

/* Allocate memory from the region. Very fast, no global tracking overhead. */
void* safe_region_alloc(safe_region_t* region, size_t size);

/* Free the entire region. The region itself is tracked by OwnedC, so 
 * failing to call this will result in a standard leak detection. */
void safe_region_free(safe_region_t* region);

#ifdef __cplusplus
}
#endif

#endif /* OWNEDC_REGION_H */
