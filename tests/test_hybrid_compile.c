#include "ownedc.h"

int main(void) {
    /* 
     * Tests __attribute__((warn_unused_result))
     * If compiled with -Werror, this file will fail to compile if the 
     * return value is ignored.
     */
    void* ptr = owner_malloc(10);
    
    owner_free(ptr);
    return 0;
}
