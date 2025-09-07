#include "ipc.h"
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <stdio.h>

typedef struct {
    void *mem;
    size_t size;
    pthread_mutex_t *mux;
    char *shm_name;
} ShmHandle;

IPC_Handle init_shm(const IPC_Config *config) {
    // Validate name length
    if (strlen(config->name) > 255) {
        fprintf(stderr, "shm name too long: %s\n", config->name);
        errno = EINVAL;
        return NULL;
    }

    // Clean up existing shared memory
    if (shm_unlink(config->name) == -1 && errno != ENOENT) {
        fprintf(stderr, "shm_unlink failed: %s\n", strerror(errno));
    }

    // Open shared memory
    int fd = shm_open(config->name, O_CREAT | O_RDWR | O_EXCL, 0666);
    if (fd == -1) {
        fprintf(stderr, "shm_open failed: %s\n", strerror(errno));
        if (errno == EEXIST) {
            fd = shm_open(config->name, O_RDWR, 0666);
            if (fd == -1) {
                fprintf(stderr, "shm_open retry failed: %s\n", strerror(errno));
                return NULL;
            }
        } else {
            return NULL;
        }
    }

    // Set size
    size_t total_size = config->size + sizeof(pthread_mutex_t);
    if (ftruncate(fd, total_size) == -1) {
        fprintf(stderr, "ftruncate failed: %s\n", strerror(errno));
        close(fd);
        shm_unlink(config->name);
        return NULL;
    }

    // Map shared memory
    void *mem = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if (mem == MAP_FAILED) {
        fprintf(stderr, "mmap failed: %s\n", strerror(errno));
        shm_unlink(config->name);
        return NULL;
    }

    // Allocate handle
    ShmHandle *h = malloc(sizeof(ShmHandle));
    if (!h) {
        fprintf(stderr, "malloc failed: %s\n", strerror(errno));
        munmap(mem, total_size);
        shm_unlink(config->name);
        return NULL;
    }

    // Initialize handle
    h->mux = (pthread_mutex_t *)mem;
    h->mem = mem + sizeof(pthread_mutex_t);
    h->size = config->size;
    h->shm_name = strdup(config->name);
    if (!h->shm_name) {
        fprintf(stderr, "strdup failed: %s\n", strerror(errno));
        free(h);
        munmap(mem, total_size);
        shm_unlink(config->name);
        return NULL;
    }

    // Initialize mutex
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    if (pthread_mutex_init(h->mux, &attr) != 0) {
        fprintf(stderr, "pthread_mutex_init failed: %s\n", strerror(errno));
        free(h->shm_name);
        free(h);
        munmap(mem, total_size);
        shm_unlink(config->name);
        return NULL;
    }
    pthread_mutexattr_destroy(&attr);

    return (IPC_Handle)h;
}

int ipc_send_shm(IPC_Handle handle, const void *data, size_t len) {
    ShmHandle *h = (ShmHandle *)handle;
    if (!h || len > h->size) {
        errno = EINVAL;
        return -1;
    }
    pthread_mutex_lock(h->mux);
    memcpy(h->mem, data, len);
    pthread_mutex_unlock(h->mux);
    return 0;
}

int ipc_recv_shm(IPC_Handle handle, void *buf, size_t len) {
    ShmHandle *h = (ShmHandle *)handle;
    if (!h || len > h->size) {
        errno = EINVAL;
        return -1;
    }
    pthread_mutex_lock(h->mux);
    memcpy(buf, h->mem, len);
    pthread_mutex_unlock(h->mux);
    return 0;
}

void close_shm(IPC_Handle handle) {
    ShmHandle *h = (ShmHandle *)handle;
    if (!h) return;
    pthread_mutex_destroy(h->mux);
    munmap(h->mux, h->size + sizeof(pthread_mutex_t));
    shm_unlink(h->shm_name);
    free(h->shm_name);
    free(h);
}
