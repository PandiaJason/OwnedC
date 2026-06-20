#include "ownedc.h"
#include "ownedc_generics.h"
#include <stdio.h>

// Define a simple struct to test with
typedef struct {
    int id;
    float value;
} my_item_t;

// Define a vector for it
OWNEDC_DEFINE_VECTOR(my_item_t, my_item_vec_t)

int main(void) {
    printf("--- OwnedC Type-Safe Generics Demo ---\n");
    
    my_item_vec_t items = my_item_vec_t_new();
    
    for (int i = 0; i < 3; i++) {
        my_item_t* item = (my_item_t*)owner_malloc(sizeof(my_item_t));
        item->id = i;
        item->value = (float)i * 1.5f;
        my_item_vec_t_push(&items, item);
    }
    
    for (size_t i = 0; i < my_item_vec_t_length(&items); i++) {
        my_item_t* borrowed = my_item_vec_t_get(&items, i);
        printf("Item %zu: id=%d, value=%.2f\n", i, borrowed->id, borrowed->value);
    }
    
    for (size_t i = 0; i < my_item_vec_t_length(&items); i++) {
        my_item_t* borrowed = my_item_vec_t_get(&items, i);
        owner_free(borrowed);
    }
    
    my_item_vec_t_free(&items);
    printf("Demo finished successfully.\n");
    return 0;
}
