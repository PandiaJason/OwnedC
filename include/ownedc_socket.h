#ifndef OWNEDC_SOCKET_H
#define OWNEDC_SOCKET_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration of the platform-specific socket descriptor type.
// We use int for POSIX, but abstract it here.
typedef int socket_fd_t;

// Safe Socket wrapper ensuring guaranteed closure via RAII
typedef struct {
    socket_fd_t fd;
    bool is_closed;
} safe_socket_t;

// Creates a new RAII wrapper for an existing raw socket descriptor
safe_socket_t* safe_socket_new(socket_fd_t raw_fd);

// Safely creates a socket (wrapper over the standard socket() call)
safe_socket_t* safe_socket_create(int domain, int type, int protocol);

// Closes the socket early (if desired).
// The macro will know not to close it again.
void safe_socket_close(safe_socket_t* sock);

// Get the raw socket descriptor for standard library operations (e.g., send/recv)
socket_fd_t safe_socket_get(safe_socket_t* sock);

// Cleanup helper for the RAII macro
static inline void cleanup_safe_socket(void* ptr) {
    safe_socket_t** sock_ptr = (safe_socket_t**)ptr;
    if (sock_ptr && *sock_ptr) {
        safe_socket_close(*sock_ptr);
    }
}

// RAII Macro specifically for network sockets
// Ensures the socket is automatically closed when dropping out of scope.
#define OWNED_SOCKET __attribute__((cleanup(cleanup_safe_socket)))

#ifdef __cplusplus
}
#endif

#endif // OWNEDC_SOCKET_H
