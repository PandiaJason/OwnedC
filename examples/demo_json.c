#include "owned_json.h"
#include "ownedc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void test_valid_json(void) {
    printf("--- [1] Parsing Valid JSON ---\n");
    const char* json_str = "{\"name\": \"Alice\", \"age\": 30.5, \"is_developer\": true, \"skills\": [\"C\", \"OwnedC\"]}";
    
    // Using the OWNED_JSON macro to bind the lifetime to the stack scope!
    OWNED_JSON json_node_t* root = json_parse(json_str, NULL);
    if (!root) {
        printf("Failed to parse valid JSON!\n");
        return;
    }
    printf("Successfully parsed JSON tree!\n");

    // Fetch and display object keys
    json_node_t* name_node = json_get_object_item(root, "name");
    json_node_t* age_node = json_get_object_item(root, "age");
    json_node_t* dev_node = json_get_object_item(root, "is_developer");
    json_node_t* skills_node = json_get_object_item(root, "skills");

    if (name_node) printf("  Name: %s\n", json_get_string(name_node));
    if (age_node) printf("  Age: %.1f\n", json_get_number(age_node));
    if (dev_node) printf("  Developer: %s\n", json_get_bool(dev_node) ? "Yes" : "No");

    if (skills_node && skills_node->type == JSON_ARRAY) {
        printf("  Skills:\n");
        for (size_t i = 0; i < skills_node->value.arr.count; i++) {
            json_node_t* skill = json_get_array_item(skills_node, i);
            printf("    - %s\n", json_get_string(skill));
        }
    }

    printf("Leaving scope: 'root' AST will now be automatically freed by OwnedC RAII.\n\n");
}

void test_invalid_json(void) {
    printf("--- [2] Parsing Invalid JSON (Error Recovery / Rollback) ---\n");
    // Mismatched brace and comma syntax error at the end
    const char* json_str = "{\"name\": \"Bob\", \"age\": 25, \"skills\": [\"C\", ";
    
    const char* error_pos = NULL;
    json_node_t* root = json_parse(json_str, &error_pos);
    if (!root) {
        printf("Parser successfully caught syntax error. Error occurred around: '%s'\n", 
               error_pos ? error_pos : "unknown");
        printf("Verifying that all partially constructed nodes were cleanly rolled back and freed.\n");
    } else {
        printf("DANGER: Parser erroneously succeeded on invalid JSON!\n");
        json_free(root);
    }
    printf("Check leaks stats: there should be 0 active parser memory leaks.\n\n");
}

void test_use_after_free(void) {
    printf("--- [3] Simulating Use-After-Free Exploit Interception ---\n");
    const char* json_str = "{\"username\": \"charlie\", \"status\": \"active\"}";
    
    json_node_t* root = json_parse(json_str, NULL);
    if (!root) return;

    json_node_t* status_node = json_get_object_item(root, "status");
    if (!status_node) return;

    // Retrieve the raw value pointer from status_node
    const char* status_val = json_get_string(status_node);
    printf("Successfully extracted nested string pointer: %s\n", status_val);

    // Let's borrow the string value pointer dynamically to register interest
    ownership_borrow((void*)status_val);

    printf("Explicitly destroying the root AST (frees status_node and status_val under the hood)...\n");
    json_free(root);

    // DANGER: We are now attempting to access the string value pointer after the AST has been freed.
    // Standard C would access garbage or crash silently. OwnedC should dynamically intercept this!
    printf("Attempting to access freed status string: %s\n", status_val);
}

int main(int argc, char* argv[]) {
    printf("==================================================\n");
    printf("    OwnedC Showcase: Memory-Safe JSON Parser      \n");
    printf("==================================================\n\n");

    if (argc > 1 && strcmp(argv[1], "--simulate-uaf") == 0) {
        test_use_after_free();
        return 0;
    }

    test_valid_json();
    test_invalid_json();

    printf("Demo finished successfully.\n");
    printf("OwnedC final stats before exit:\n");
    ownership_stats();

    return 0;
}
