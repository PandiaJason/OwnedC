#include "ownedc.h"
#include "ownedc_socket.h"
#include <stdio.h>
#include <sys/socket.h>

void connect_to_server_safely(bool simulate_error) {
    // 1. Create the socket safely
    OWNED_SOCKET safe_socket_t* sock = safe_socket_create(AF_INET, SOCK_STREAM, 0);
    if (!sock) {
        printf("Failed to create socket!\n");
        return;
    }
    
    printf("Socket %d created successfully.\n", safe_socket_get(sock));
    
    if (simulate_error) {
        printf("Simulating a connection error! Returning early...\n");
        return; // DANGER: Standard C leaks the socket here. OwnedC closes it automatically.
    }
    
    printf("Connection logic would go here...\n");
    printf("Finished normally. Socket will be closed.\n");
}

int main(void) {
    printf("--- OwnedC Safe Socket Demo ---\n");
    
    printf("\nExecuting normal network task:\n");
    connect_to_server_safely(false);
    
    printf("\nExecuting network task that encounters an error:\n");
    connect_to_server_safely(true);
    
    return 0;
}
