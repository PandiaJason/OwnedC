#include "ownedc_socket.h"
#include "ownedc.h"
#include <unistd.h>
#include <sys/socket.h>

safe_socket_t* safe_socket_new(socket_fd_t raw_fd) {
    if (raw_fd < 0) return NULL;
    
    safe_socket_t* sock = (safe_socket_t*)owner_malloc(sizeof(safe_socket_t));
    if (!sock) return NULL;
    
    sock->fd = raw_fd;
    sock->is_closed = false;
    
    return sock;
}

safe_socket_t* safe_socket_create(int domain, int type, int protocol) {
    socket_fd_t raw_fd = socket(domain, type, protocol);
    if (raw_fd < 0) return NULL;
    
    safe_socket_t* sock = safe_socket_new(raw_fd);
    if (!sock) {
        close(raw_fd);
        return NULL;
    }
    return sock;
}

void safe_socket_close(safe_socket_t* sock) {
    if (!sock) return;
    
    if (!sock->is_closed) {
        if (sock->fd >= 0) {
            close(sock->fd);
        }
        sock->is_closed = true;
    }
    
    owner_free(sock);
}

socket_fd_t safe_socket_get(safe_socket_t* sock) {
    if (!sock || sock->is_closed) {
        return -1;
    }
    return sock->fd;
}
