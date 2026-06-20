#include "ownedc_array.h"
#include "ownedc.h"
#include "ownedc_env.h"


owned_array_t* owned_array_new(size_t capacity) {
    owned_array_t* arr = (owned_array_t*)owner_malloc(sizeof(owned_array_t));
    if (!arr) return NULL;
    
    arr->capacity = capacity;
    if (capacity > 0) {
        // Allocate the underlying array of void* pointers.
        // We use calloc to ensure all pointers start as NULL.
        arr->data = (void**)owner_calloc(capacity, sizeof(void*));
        if (!arr->data) {
            owner_free(arr);
            return NULL;
        }
    } else {
        arr->data = NULL;
    }
    
    return arr;
}

void owned_array_set(owned_array_t* arr, size_t index, void* ptr) {
    if (!arr) return;
    
    if (index >= arr->capacity) {
        OWNEDC_PRINTF("OWNEDC FATAL: Array bounds violation! Index %zu, Capacity %zu\n", index, arr->capacity);
        OWNEDC_ABORT();
    }
    
    // If there is already a pointer there, we should free it to prevent leaks!
    if (arr->data[index] != NULL && arr->data[index] != ptr) {
        owner_free(arr->data[index]);
    }
    
    arr->data[index] = ptr;
}

void* owned_array_get(owned_array_t* arr, size_t index) {
    if (!arr) return NULL;
    
    if (index >= arr->capacity) {
        OWNEDC_PRINTF("OWNEDC FATAL: Array bounds violation! Index %zu, Capacity %zu\n", index, arr->capacity);
        OWNEDC_ABORT();
    }
    
    return arr->data[index];
}

void owned_array_free(owned_array_t* arr) {
    if (!arr) return;
    
    if (arr->data) {
        // THE MAGIC: Deep free!
        // Loop through the array and free every non-null pointer.
        for (size_t i = 0; i < arr->capacity; i++) {
            if (arr->data[i] != NULL) {
                // Free the inner element using OwnedC's registry!
                owner_free(arr->data[i]);
                arr->data[i] = NULL;
            }
        }
        
        // Free the internal data buffer
        owner_free(arr->data);
        arr->data = NULL;
    }
    
    // Free the array struct itself
    owner_free(arr);
}
