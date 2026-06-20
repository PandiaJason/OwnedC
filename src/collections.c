#include "ownedc_collections.h"
#include "ownedc_env.h"


extern void ownedc_diagnostics_fatal(const char* reason, void* ptr, const char* file, int line);

safe_vector_t* safe_vector_create(size_t initial_capacity) {
    if (initial_capacity == 0) initial_capacity = 4;
    
    safe_vector_t* vec = (safe_vector_t*)owner_malloc(sizeof(safe_vector_t));
    if (!vec) return NULL;
    
    vec->data = (void**)owner_malloc(initial_capacity * sizeof(void*));
    if (!vec->data) {
        owner_free(vec);
        return NULL;
    }
    
    vec->length = 0;
    vec->capacity = initial_capacity;
    return vec;
}

void safe_vector_push(safe_vector_t* vec, void* item) {
    if (!vec) return;
    
    if (vec->length >= vec->capacity) {
        size_t new_cap = vec->capacity * 2;
        void** new_data = (void**)owner_realloc(vec->data, new_cap * sizeof(void*));
        if (!new_data) {
            ownedc_diagnostics_fatal("Failed to reallocate safe_vector", vec, __FILE__, __LINE__);
        }
        vec->data = new_data;
        vec->capacity = new_cap;
    }
    
    vec->data[vec->length++] = item;
}

void* safe_vector_get(safe_vector_t* vec, size_t index) {
    if (!vec) return NULL;
    
    if (index >= vec->length) {
        // Out of bounds detection
        ownedc_diagnostics_fatal("Out-of-bounds safe_vector access", vec, __FILE__, __LINE__);
    }
    
    return vec->data[index];
}

void safe_vector_free(safe_vector_t* vec) {
    if (!vec) return;
    if (vec->data) {
        owner_free(vec->data);
    }
    owner_free(vec);
}

void safe_vector_remove(safe_vector_t* vec, size_t index) {
    if (!vec || !vec->data) {
        OWNEDC_PRINTF("OWNEDC FATAL: Null vector access.\n");
        OWNEDC_ABORT();
    }
    if (index >= vec->length) {
        OWNEDC_PRINTF("OWNEDC FATAL: Vector bounds violation! Index %zu, Length %zu\n", index, vec->length);
        OWNEDC_ABORT();
    }
    
    // Shift elements left
    for (size_t i = index; i < vec->length - 1; i++) {
        vec->data[i] = vec->data[i + 1];
    }
    vec->length--;
}
