#include "ipc.h"        // For IPC_Handle, IPC_Config
#include <sys/mman.h>   // For shm_open, mmap, munmap
#include <fcntl.h>      // For O_CREAT, O_RDWR, ftruncate
#include <unistd.h>     // For close, shm_unlink
#include <stdlib.h>     // For malloc, free
#include <string.h>     // For memcpy
#include <pthread.h>    // For pthread_mutex_t
#include <errno.h>      // For errno, EINVAL

typedef struct {
    void *mem;          // Shared memory data region
    size_t size;        // Size of data region
    pthread_mutex_t *mux; // Mutex in shared memory
    char *shm_name;     // Store shared memory name for cleanup
} ShmHandle;

IPC_Handle init_shm(const IPC_Config *config) {
    // Open shared memory
    int fd = shm_open(config->name, O_CREAT | O_RDWR, 0600); // macOS prefers 0600
    if (fd == -1) {
        return NULL;
    }

    // Set size (mutex + data)
    size_t total_size = config->size + sizeof(pthread_mutex_t);
    if (ftruncate(fd, total_size) == -1) {
        close(fd);
        shm_unlink(config->name);
        return NULL;
    }

    // Map shared memory
    void *mem = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd); // Close fd after mapping
    if (mem == MAP_FAILED) {
        shm_unlink(config->name);
        return NULL;
    }

    // Allocate handle
    ShmHandle *h = malloc(sizeof(ShmHandle));
    if (!h) {
        munmap(mem, total_size);
        shm_unlink(config->name);
        return NULL;
    }

    // Initialize handle
    h->mux = (pthread_mutex_t *)mem;
    h->mem = mem + sizeof(pthread_mutex_t);
    h->size = config->size;
    h->shm_name = strdup(config->name); // Store name for cleanup
    if (!h->shm_name) {
        free(h);
        munmap(mem, total_size);
        shm_unlink(config->name);
        return NULL;
    }

    // Initialize mutex
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(h->mux, &attr);
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
    shm_unlink(h->shm_name); // Use stored name
    free(h->shm_name);
    free(h);
}
