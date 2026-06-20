#include "ownedc_file.h"
#include "ownedc.h"
#include "ownedc_env.h"


safe_file_t* safe_fopen(const char* filename, const char* mode) {
    FILE* raw_file = fopen(filename, mode);
    if (!raw_file) {
        return NULL;
    }
    
    // Allocate the wrapper using the OwnedC registry
    safe_file_t* safe_file = (safe_file_t*)owner_malloc(sizeof(safe_file_t));
    if (!safe_file) {
        fclose(raw_file);
        return NULL;
    }
    
    safe_file->handle = raw_file;
    safe_file->is_closed = false;
    
    return safe_file;
}

void safe_fclose(safe_file_t* file) {
    if (!file) return;
    
    // Prevent double closes
    if (!file->is_closed) {
        if (file->handle) {
            fclose(file->handle);
        }
        file->is_closed = true;
    }
    
    // Free the wrapper itself from the OwnedC registry
    owner_free(file);
}

FILE* safe_file_get(safe_file_t* file) {
    if (!file || file->is_closed) {
        return NULL;
    }
    return file->handle;
}
