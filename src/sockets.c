#include "ipc.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

typedef struct {
    int sock;
    int client_sock;  // For accepted connections
    char *sock_path;  // For Unix socket cleanup
} SockHandle;

IPC_Handle init_socket_unix(const IPC_Config *config) {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1) return NULL;

    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, config->name, sizeof(addr.sun_path) - 1);
    unlink(config->name);

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        close(sock);
        return NULL;
    }
    if (listen(sock, 5) == -1) {
        close(sock);
        return NULL;
    }

    SockHandle *h = malloc(sizeof(SockHandle));
    if (!h) {
        close(sock);
        return NULL;
    }
    h->sock = sock;
    h->client_sock = -1;
    h->sock_path = strdup(config->name);
    if (!h->sock_path) {
        free(h);
        close(sock);
        return NULL;
    }
    return (IPC_Handle)h;
}

int ipc_send_socket_unix(IPC_Handle handle, const void *data, size_t len) {
    SockHandle *h = (SockHandle *)handle;
    if (!h || !data || len == 0) {
        errno = EINVAL;
        return -1;
    }
    if (h->client_sock == -1) {
        h->client_sock = accept(h->sock, NULL, NULL);
        if (h->client_sock == -1) return -1;
    }
    return send(h->client_sock, data, len, 0);
}

int ipc_recv_socket_unix(IPC_Handle handle, void *buf, size_t len) {
    SockHandle *h = (SockHandle *)handle;
    if (!h || !buf || len == 0) {
        errno = EINVAL;
        return -1;
    }
    if (h->client_sock == -1) {
        h->client_sock = accept(h->sock, NULL, NULL);
        if (h->client_sock == -1) return -1;
    }
    return recv(h->client_sock, buf, len, 0);
}

void close_socket_unix(IPC_Handle handle) {
    SockHandle *h = (SockHandle *)handle;
    if (!h) return;
    if (h->client_sock != -1) close(h->client_sock);
    close(h->sock);
    if (h->sock_path) {
        unlink(h->sock_path);
        free(h->sock_path);
    }
    free(h);
}

IPC_Handle init_socket_tcp(const IPC_Config *config) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) return NULL;

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(config->port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        close(sock);
        return NULL;
    }
    if (listen(sock, 5) == -1) {
        close(sock);
        return NULL;
    }

    SockHandle *h = malloc(sizeof(SockHandle));
    if (!h) {
        close(sock);
        return NULL;
    }
    h->sock = sock;
    h->client_sock = -1;
    h->sock_path = NULL;
    return (IPC_Handle)h;
}

int ipc_send_socket_tcp(IPC_Handle handle, const void *data, size_t len) {
    return ipc_send_socket_unix(handle, data, len);
}

int ipc_recv_socket_tcp(IPC_Handle handle, void *buf, size_t len) {
    return ipc_recv_socket_unix(handle, buf, len);
}

void close_socket_tcp(IPC_Handle handle) {
    close_socket_unix(handle);
}
