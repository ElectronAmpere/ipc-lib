#include <sys/socket.h>
#include <sys/un.h>

typedef struct {
    int sock;
    int client_sock;  // For connected
} SockHandle;

IPC_Handle init_socket_unix(const IPC_Config *config) {  // Server side
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr = {.sun_family = AF_UNIX};
    strncpy(addr.sun_path, config->name, sizeof(addr.sun_path)-1);
    unlink(config->name);  // Cleanup
    bind(sock, (struct sockaddr*)&addr, sizeof(addr));
    listen(sock, 5);  // Backlog

    SockHandle *h = malloc(sizeof(SockHandle));
    h->sock = sock;
    h->client_sock = -1;
    return h;
}

// Client: Use connect instead of bind/listen.

// For many: In send/recv, accept() first if server.
int ipc_send_sock(SockHandle *h, const void *data, size_t len) {
    if (h->client_sock == -1) h->client_sock = accept(h->sock, NULL, NULL);
    return send(h->client_sock, data, len, 0);
}

int ipc_recv_sock(SockHandle *h, void *buf, size_t len) {
    if (h->client_sock == -1) h->client_sock = accept(h->sock, NULL, NULL);
    return recv(h->client_sock, buf, len, 0);
}

void close_sock(SockHandle *h) {
    close(h->client_sock);
    close(h->sock);
    unlink(((IPC_Config*)/*...*/)->name);
    free(h);
}
