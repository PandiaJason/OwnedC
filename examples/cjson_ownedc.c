#include "cJSON.h"
#include "ownedc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[]) {
    printf("==================================================\n");
    printf("     OwnedC Showcase: Memory-Safe cJSON Parser     \n");
    printf("==================================================\n\n");

    // 1. Initialize cJSON hooks to point to OwnedC's memory tracker!
    cJSON_Hooks hooks;
    hooks.malloc_fn = owner_malloc;
    hooks.free_fn = owner_free;
    cJSON_InitHooks(&hooks);
    printf("[1] Successfully bound cJSON hooks to OwnedC registry.\n");

    // 2. Parse a valid JSON string
    const char* json_str = "{\"name\": \"Alice\", \"age\": 30, \"languages\": [\"C\", \"OwnedC\"], \"active\": true}";
    cJSON* root = cJSON_Parse(json_str);
    if (!root) {
        fprintf(stderr, "Error parsing JSON: %s\n", cJSON_GetErrorPtr());
        return 1;
    }
    printf("[2] Parsed JSON string into AST successfully.\n");

    // Print active allocations tracked by OwnedC
    printf("  > Active allocations right after JSON parsing:\n");
    ownership_stats();

    // 3. Extract items
    cJSON* name = cJSON_GetObjectItemCaseSensitive(root, "name");
    cJSON* age = cJSON_GetObjectItemCaseSensitive(root, "age");
    cJSON* languages = cJSON_GetObjectItemCaseSensitive(root, "languages");
    
    if (cJSON_IsString(name) && (name->valuestring != NULL)) {
        printf("  Name: %s\n", name->valuestring);
    }
    if (cJSON_IsNumber(age)) {
        printf("  Age: %d\n", age->valueint);
    }
    if (cJSON_IsArray(languages)) {
        printf("  Languages:\n");
        int size = cJSON_GetArraySize(languages);
        for (int i = 0; i < size; i++) {
            cJSON* item = cJSON_GetArrayItem(languages, i);
            printf("    - %s\n", item->valuestring);
        }
    }

    // 4. Simulate a memory leak
    if (argc > 1 && strcmp(argv[1], "--simulate-leak") == 0) {
        printf("\n[DANGER] Simulating a memory leak (forgetting to call cJSON_Delete)...\n");
        // We omit cJSON_Delete(root)
        printf("[DANGER] Program ending now. OwnedC's exit checker should catch the leak!\n");
        return 0;
    }

    // 5. Simulate a double-free
    if (argc > 1 && strcmp(argv[1], "--simulate-double-free") == 0) {
        printf("\n[DANGER] Simulating a double-free on a cJSON allocation...\n");
        void* temp = cJSON_malloc(64);
        cJSON_free(temp);
        cJSON_free(temp); // Double-free!
        return 0;
    }

    // 6. Clean up normally
    cJSON_Delete(root);
    printf("\n[3] Cleaned up JSON AST using cJSON_Delete.\n");

    printf("[4] Program finished successfully.\n");
    printf("  > OwnedC stats before normal exit:\n");
    ownership_stats();

    return 0;
}
