#include <sys/stat.h>
#include <fcntl.h>

typedef struct {
    int fd;
} PipeHandle;

IPC_Handle init_pipe_named(const IPC_Config *config) {
    mkfifo(config->name, 0666);
    int fd = open(config->name, O_RDWR | O_NONBLOCK);  // Non-block optional
    PipeHandle *h = malloc(sizeof(PipeHandle));
    h->fd = fd;
    return h;
}

int ipc_send_pipe(PipeHandle *h, const void *data, size_t len) {
    return write(h->fd, data, len);
}

int ipc_recv_pipe(PipeHandle *h, void *buf, size_t len) {
    return read(h->fd, buf, len);
}

void close_pipe(PipeHandle *h) {
    close(h->fd);
    unlink(((IPC_Config*)/*...*/)->name);
    free(h);
}
