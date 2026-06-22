#include "cJSON.h"
#include "ownedc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// libxml2 headers
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlmemory.h>

// OpenSSL headers
#include <openssl/crypto.h>
#include <openssl/evp.h>

// libxml2 custom strdup implementation using owner_malloc
static char* owned_xml_strdup(const char* str) {
    if (!str) return NULL;
    size_t len = strlen(str);
    char* copy = (char*)owner_malloc(len + 1);
    if (copy) {
        memcpy(copy, str, len + 1);
    }
    return copy;
}

// OpenSSL allocations wrapper to pass file & line to OwnedC
static void* openssl_malloc(size_t num, const char* file, int line) {
    return owner_malloc_internal(num, file, line);
}

static void* openssl_realloc(void* addr, size_t num, const char* file, int line) {
    return owner_realloc_internal(addr, num, file, line);
}

static void openssl_free(void* addr, const char* file, int line) {
    owner_free_internal(addr, file, line);
}

// 1. cJSON Test
void run_cjson_test(void) {
    printf("\n--- [1] Running cJSON Memory-Safe Test ---\n");
    cJSON_Hooks hooks;
    hooks.malloc_fn = owner_malloc;
    hooks.free_fn = owner_free;
    cJSON_InitHooks(&hooks);

    const char* json_str = "{\"project\": \"OwnedC\", \"language\": \"C\"}";
    cJSON* root = cJSON_Parse(json_str);
    if (root) {
        cJSON* proj = cJSON_GetObjectItem(root, "project");
        printf("  Parsed JSON: Project = %s\n", proj->valuestring);
        cJSON_Delete(root);
    }
}

// 2. libxml2 Test
void run_libxml2_test(void) {
    printf("\n--- [2] Running libxml2 Memory-Safe Test ---\n");
    
    // Bind libxml2 memory methods to OwnedC
    xmlMemSetup(owner_free, owner_malloc, owner_realloc, owned_xml_strdup);

    const char* xml_str = "<note><to>World</to></note>";
    xmlDocPtr doc = xmlReadMemory(xml_str, strlen(xml_str), "noname.xml", NULL, 0);
    if (doc) {
        xmlNodePtr root_element = xmlDocGetRootElement(doc);
        printf("  Parsed XML: Root Element = %s\n", root_element->name);
        xmlFreeDoc(doc);
    }
    xmlCleanupParser();
}

// 3. OpenSSL Test
void run_openssl_test(void) {
    printf("\n--- [3] Running OpenSSL Memory-Safe Test ---\n");

    // Bind OpenSSL memory methods to OwnedC, providing code line context!
    CRYPTO_set_mem_functions(openssl_malloc, openssl_realloc, openssl_free);

    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (mdctx) {
        const EVP_MD* md = EVP_sha256();
        unsigned char md_value[EVP_MAX_MD_SIZE];
        unsigned int md_len;
        const char* msg = "Memory Safety in C";

        EVP_DigestInit_ex(mdctx, md, NULL);
        EVP_DigestUpdate(mdctx, msg, strlen(msg));
        EVP_DigestFinal_ex(mdctx, md_value, &md_len);

        printf("  SHA256 hash successfully generated!\n");
        printf("  Hash value: ");
        for (unsigned int i = 0; i < md_len; i++) {
            printf("%02x", md_value[i]);
        }
        printf("\n");

        EVP_MD_CTX_free(mdctx);
    }
}

int main(int argc, char* argv[]) {
    printf("==================================================\n");
    printf("     OwnedC Showcase: Real System Libraries       \n");
    printf("   (cJSON, libxml2, and OpenSSL Memory Hooks)     \n");
    printf("==================================================\n");

    run_cjson_test();
    run_libxml2_test();
    run_openssl_test();

    printf("\n==================================================\n");
    printf("  Summary: Clean normal execution finished.\n");
    printf("==================================================\n");
    ownership_stats();

    // Check for simulated leak in OpenSSL context
    if (argc > 1 && strcmp(argv[1], "--simulate-leak") == 0) {
        printf("\n[DANGER] Simulating a memory leak (forgetting to free OpenSSL ctx)...\n");
        EVP_MD_CTX* mdctx = EVP_MD_CTX_new(); // Left unreferenced
        (void)mdctx;
        return 0;
    }

    return 0;
}
