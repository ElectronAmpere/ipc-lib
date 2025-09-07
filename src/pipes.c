#include "ipc.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

typedef struct {
    int read_fd;
    int write_fd;
    char *fifo_name;  // NULL for unnamed
} PipeHandle;

IPC_Handle init_pipe_named(const IPC_Config *config) {
    if (mkfifo(config->name, 0600) == -1 && errno != EEXIST) {
        return NULL;
    }

    PipeHandle *h = malloc(sizeof(PipeHandle));
    if (!h) return NULL;

    h->write_fd = open(config->name, O_WRONLY | O_NONBLOCK);
    if (h->write_fd == -1) {
        free(h);
        return NULL;
    }
    h->read_fd = open(config->name, O_RDONLY | O_NONBLOCK);
    if (h->read_fd == -1) {
        close(h->write_fd);
        free(h);
        return NULL;
    }
    h->fifo_name = strdup(config->name);
    if (!h->fifo_name) {
        close(h->read_fd);
        close(h->write_fd);
        free(h);
        return NULL;
    }
    return (IPC_Handle)h;
}

int ipc_send_pipe_named(IPC_Handle handle, const void *data, size_t len) {
    PipeHandle *h = (PipeHandle *)handle;
    if (!h || !data || len == 0) {
        errno = EINVAL;
        return -1;
    }
    return write(h->write_fd, data, len);
}

int ipc_recv_pipe_named(IPC_Handle handle, void *buf, size_t len) {
    PipeHandle *h = (PipeHandle *)handle;
    if (!h || !buf || len == 0) {
        errno = EINVAL;
        return -1;
    }
    return read(h->read_fd, buf, len);
}

void close_pipe_named(IPC_Handle handle) {
    PipeHandle *h = (PipeHandle *)handle;
    if (!h) return;
    close(h->read_fd);
    close(h->write_fd);
    if (h->fifo_name) {
        unlink(h->fifo_name);
        free(h->fifo_name);
    }
    free(h);
}

IPC_Handle init_pipe_unnamed(const IPC_Config *config) {
    int pipefds[2];
    if (pipe(pipefds) == -1) {
        return NULL;
    }

    PipeHandle *h = malloc(sizeof(PipeHandle));
    if (!h) {
        close(pipefds[0]);
        close(pipefds[1]);
        return NULL;
    }
    h->read_fd = pipefds[0];
    h->write_fd = pipefds[1];
    h->fifo_name = NULL;
    return (IPC_Handle)h;
}

int ipc_send_pipe_unnamed(IPC_Handle handle, const void *data, size_t len) {
    return ipc_send_pipe_named(handle, data, len);
}

int ipc_recv_pipe_unnamed(IPC_Handle handle, void *buf, size_t len) {
    return ipc_recv_pipe_named(handle, buf, len);
}

void close_pipe_unnamed(IPC_Handle handle) {
    close_pipe_named(handle);
}
