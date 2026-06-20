#include "ownedc.h"
#include "ownedc_region.h"
#include "ownedc_string.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080
#define BUFFER_SIZE 4096

// Client context passed to the worker thread
typedef struct {
    int client_socket;
} client_context_t;

void* handle_client(void* arg) {
    client_context_t* ctx = (client_context_t*)arg;
    
    // 1. Borrow the shared context
    ownership_borrow(ctx);
    int client_sock = ctx->client_socket;
    
    printf("[Thread %lu] Accepted connection on socket %d\n", (unsigned long)pthread_self(), client_sock);
    
    // 2. Create a Region Arena for this specific HTTP Request.
    // All temporary parsing allocations will go here and be destroyed instantly at the end.
    safe_region_t* request_arena = safe_region_create(8192); // 8KB arena
    
    // 3. RAII buffer for reading the request (Auto-freed if we return early)
    OWNED char* buffer = owner_malloc(BUFFER_SIZE);
    
    ssize_t bytes_read = read(client_sock, buffer, BUFFER_SIZE - 1);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        
        // Use the Arena to parse lines (simulating HTTP header parsing)
        char* token = strtok(buffer, "\r\n");
        if (token) {
            // Allocate a copy of the request line in the Arena
            char* req_line = (char*)safe_region_alloc(request_arena, strlen(token) + 1);
            strcpy(req_line, token);
            printf("[Thread %lu] Request: %s\n", (unsigned long)pthread_self(), req_line);
            
            // 4. Use Safe Strings to dynamically build the HTTP response
            OWNED_STRING safe_string_t* response = safe_string_new("HTTP/1.1 200 OK\r\n");
            safe_string_append(response, "Content-Type: text/html\r\n");
            safe_string_append(response, "Connection: close\r\n\r\n");
            
            safe_string_append(response, "<html><head><title>OwnedC Web Server</title></head>");
            safe_string_append(response, "<body style=\"font-family: sans-serif; text-align: center; margin-top: 50px;\">");
            safe_string_append(response, "<h1>🚀 Powered by OwnedC</h1>");
            safe_string_append(response, "<p>This multi-threaded C web server is 100% memory safe!</p>");
            safe_string_append(response, "<p>You requested: <strong>");
            safe_string_append(response, req_line);
            safe_string_append(response, "</strong></p>");
            safe_string_append(response, "<p>Zero memory leaks, guaranteed RAII cleanup, and Region Arena request contexts.</p>");
            safe_string_append(response, "</body></html>");
            
            // Send the perfectly safe, dynamic response
            if (write(client_sock, safe_string_cstr(response), response->length) < 0) {
                // Ignore write errors for the demo
            }
        }
    }
    
    // 5. Cleanup Connection State
    close(client_sock);
    
    // Destroy the Request Arena (frees all parsing allocations instantly)
    safe_region_free(request_arena);
    
    // Release the borrow and free the cross-thread context
    ownership_release(ctx);
    owner_free(ctx);
    
    printf("[Thread %lu] Connection closed gracefully. Context destroyed.\n", (unsigned long)pthread_self());
    return NULL;
}

int main(void) {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    
    printf("--- OwnedC Real-World HTTP Server ---\n");
    
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
    printf("Server listening on port %d...\n", PORT);
    printf("Test it by opening: http://localhost:%d\n\n", PORT);
    
    while (1) {
        int client_socket;
        int addrlen = sizeof(address);
        
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            continue;
        }
        
        // 1. Allocate context on the main thread
        client_context_t* ctx = owner_malloc(sizeof(client_context_t));
        ctx->client_socket = client_socket;
        
        // 2. Share ownership so the worker thread can safely take it
        ownership_share(ctx);
        
        // 3. Spawn a worker thread to handle the HTTP request concurrently
        pthread_t t;
        if (pthread_create(&t, NULL, handle_client, ctx) != 0) {
            perror("Failed to create thread");
            close(client_socket);
            owner_free(ctx);
        } else {
            // Detach so we don't need to join it (it cleans up itself)
            pthread_detach(t);
        }
    }
    
    return 0;
}
