#include "ownedc_rc.h"
#include "ownedc.h"
#include <stdio.h>
#include <stdlib.h>

owned_rc_t* owned_rc_new(void* ptr) {
    owned_rc_t* rc = (owned_rc_t*)owner_malloc(sizeof(owned_rc_t));
    if (!rc) {
        if (ptr) owner_free(ptr); // Prevent leak if wrapper allocation fails
        return NULL;
    }
    
    rc->ptr = ptr;
    rc->ref_count = 1; // Initial reference count
    
    return rc;
}

owned_rc_t* owned_rc_clone(owned_rc_t* rc) {
    if (!rc) return NULL;
    
    // Increment the reference count
    rc->ref_count++;
    return rc;
}

void owned_rc_drop(owned_rc_t* rc) {
    if (!rc) return;
    
    if (rc->ref_count > 0) {
        rc->ref_count--;
        
        // Zero leaks on zero refs!
        if (rc->ref_count == 0) {
            if (rc->ptr) {
                owner_free(rc->ptr); // Free the inner data
                rc->ptr = NULL;
            }
            
            // Free the reference counting wrapper itself
            owner_free(rc);
        }
    }
}

void* owned_rc_get(owned_rc_t* rc) {
    if (!rc) return NULL;
    return rc->ptr;
}
