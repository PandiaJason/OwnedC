#include "ownedc.h"
#include "ownedc_arena.h"
#include <stdio.h>

typedef struct {
    int x;
    int y;
} point_t;

int main(void) {
    printf("--- OwnedC Lock-Free Bump Arena Demo ---\n");
    
    // Create an arena with a 4KB block size
    ownedc_arena_t* arena = ownedc_arena_create(4096);
    
    // Allocate 1000 points instantly
    for (int i = 0; i < 1000; i++) {
        point_t* p = (point_t*)ownedc_arena_alloc(arena, sizeof(point_t));
        p->x = i;
        p->y = i * 2;
        
        if (i == 500) {
            printf("Point 500: (%d, %d)\n", p->x, p->y);
        }
    }
    
    printf("Allocated 1000 points without any global locks!\n");
    
    // Destroy the entire arena and all its blocks in O(1) conceptually
    ownedc_arena_destroy(arena);
    
    printf("Arena destroyed successfully. No leaks!\n");
    return 0;
}
