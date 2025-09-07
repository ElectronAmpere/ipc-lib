#include "ipc.h"
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

// Forward declarations of mechanism-specific functions
// Shared Memory (shm_mutex.c)
IPC_Handle init_shm(const IPC_Config *config);
int ipc_send_shm(IPC_Handle handle, const void *data, size_t len);
int ipc_recv_shm(IPC_Handle handle, void *buf, size_t len);
void close_shm(IPC_Handle handle);

// POSIX Message Queue (msg_queue.c)
#ifdef __linux__
IPC_Handle init_mq_posix(const IPC_Config *config);
int ipc_send_mq_posix(IPC_Handle handle, const void *data, size_t len);
int ipc_recv_mq_posix(IPC_Handle handle, void *buf, size_t len);
void close_mq_posix(IPC_Handle handle);
#endif

// System V Message Queue (msg_queue.c)
#ifdef __linux__
IPC_Handle init_mq_sysv(const IPC_Config *config);
int ipc_send_mq_sysv(IPC_Handle handle, const void *data, size_t len);
int ipc_recv_mq_sysv(IPC_Handle handle, void *buf, size_t len);
void close_mq_sysv(IPC_Handle handle);
#endif

// Named Pipe (pipes.c)
IPC_Handle init_pipe_named(const IPC_Config *config);
int ipc_send_pipe_named(IPC_Handle handle, const void *data, size_t len);
int ipc_recv_pipe_named(IPC_Handle handle, void *buf, size_t len);
void close_pipe_named(IPC_Handle handle);

// Unnamed Pipe (pipes.c)
IPC_Handle init_pipe_unnamed(const IPC_Config *config);
int ipc_send_pipe_unnamed(IPC_Handle handle, const void *data, size_t len);
int ipc_recv_pipe_unnamed(IPC_Handle handle, void *buf, size_t len);
void close_pipe_unnamed(IPC_Handle handle);

// Unix Domain Socket (sockets.c)
IPC_Handle init_socket_unix(const IPC_Config *config);
int ipc_send_socket_unix(IPC_Handle handle, const void *data, size_t len);
int ipc_recv_socket_unix(IPC_Handle handle, void *buf, size_t len);
void close_socket_unix(IPC_Handle handle);

// TCP/IP Socket (sockets.c)
IPC_Handle init_socket_tcp(const IPC_Config *config);
int ipc_send_socket_tcp(IPC_Handle handle, const void *data, size_t len);
int ipc_recv_socket_tcp(IPC_Handle handle, void *buf, size_t len);
void close_socket_tcp(IPC_Handle handle);

// Internal handle structure
typedef struct {
    IPC_Mechanism mech;
    IPC_Handle mech_handle;
} IPC_CoreHandle;

// Initialize IPC based on config
IPC_Handle ipc_init(const IPC_Config *config) {
    if (!config || !config->name || config->size == 0) {
        errno = EINVAL;
        return NULL;
    }

    IPC_CoreHandle *core_h = malloc(sizeof(IPC_CoreHandle));
    if (!core_h) {
        errno = ENOMEM;
        return NULL;
    }
    core_h->mech = config->mech;

    switch (core_h->mech) {
        case IPC_SHM_MUTEX:
            core_h->mech_handle = init_shm(config);
            break;
#ifdef __linux__
        case IPC_MQ_POSIX:
            core_h->mech_handle = init_mq_posix(config);
            break;
        case IPC_MQ_SYSV:
            core_h->mech_handle = init_mq_sysv(config);
            break;
#else
        case IPC_MQ_POSIX:
        case IPC_MQ_SYSV:
            errno = ENOTSUP;
            free(core_h);
            return NULL;
#endif
        case IPC_PIPE_NAMED:
            core_h->mech_handle = init_pipe_named(config);
            break;
        case IPC_PIPE_UNNAMED:
            core_h->mech_handle = init_pipe_unnamed(config);
            break;
        case IPC_SOCKET_UNIX:
            core_h->mech_handle = init_socket_unix(config);
            break;
        case IPC_SOCKET_TCP:
            core_h->mech_handle = init_socket_tcp(config);
            break;
        default:
            free(core_h);
            errno = EINVAL;
            return NULL;
    }

    if (!core_h->mech_handle) {
        free(core_h);
        return NULL;
    }
    return core_h;
}

// Send data via the appropriate mechanism
int ipc_send(IPC_Handle handle, const void *data, size_t len) {
    if (!handle || !data || len == 0) {
        errno = EINVAL;
        return -1;
    }

    IPC_CoreHandle *core_h = (IPC_CoreHandle *)handle;
    switch (core_h->mech) {
        case IPC_SHM_MUTEX:
            return ipc_send_shm(core_h->mech_handle, data, len);
#ifdef __linux__
        case IPC_MQ_POSIX:
            return ipc_send_mq_posix(core_h->mech_handle, data, len);
        case IPC_MQ_SYSV:
            return ipc_send_mq_sysv(core_h->mech_handle, data, len);
#endif
        case IPC_PIPE_NAMED:
            return ipc_send_pipe_named(core_h->mech_handle, data, len);
        case IPC_PIPE_UNNAMED:
            return ipc_send_pipe_unnamed(core_h->mech_handle, data, len);
        case IPC_SOCKET_UNIX:
            return ipc_send_socket_unix(core_h->mech_handle, data, len);
        case IPC_SOCKET_TCP:
            return ipc_send_socket_tcp(core_h->mech_handle, data, len);
        default:
            errno = EINVAL;
            return -1;
    }
}

// Receive data via the appropriate mechanism
int ipc_recv(IPC_Handle handle, void *buf, size_t len) {
    if (!handle || !buf || len == 0) {
        errno = EINVAL;
        return -1;
    }

    IPC_CoreHandle *core_h = (IPC_CoreHandle *)handle;
    switch (core_h->mech) {
        case IPC_SHM_MUTEX:
            return ipc_recv_shm(core_h->mech_handle, buf, len);
#ifdef __linux__
        case IPC_MQ_POSIX:
            return ipc_recv_mq_posix(core_h->mech_handle, buf, len);
        case IPC_MQ_SYSV:
            return ipc_recv_mq_sysv(core_h->mech_handle, buf, len);
#endif
        case IPC_PIPE_NAMED:
            return ipc_recv_pipe_named(core_h->mech_handle, buf, len);
        case IPC_PIPE_UNNAMED:
            return ipc_recv_pipe_unnamed(core_h->mech_handle, buf, len);
        case IPC_SOCKET_UNIX:
            return ipc_recv_socket_unix(core_h->mech_handle, buf, len);
        case IPC_SOCKET_TCP:
            return ipc_recv_socket_tcp(core_h->mech_handle, buf, len);
        default:
            errno = EINVAL;
            return -1;
    }
}

// Close and cleanup IPC resources
void ipc_close(IPC_Handle handle) {
    if (!handle) return;

    IPC_CoreHandle *core_h = (IPC_CoreHandle *)handle;
    switch (core_h->mech) {
        case IPC_SHM_MUTEX:
            close_shm(core_h->mech_handle);
            break;
#ifdef __linux__
        case IPC_MQ_POSIX:
            close_mq_posix(core_h->mech_handle);
            break;
        case IPC_MQ_SYSV:
            close_mq_sysv(core_h->mech_handle);
            break;
#endif
        case IPC_PIPE_NAMED:
            close_pipe_named(core_h->mech_handle);
            break;
        case IPC_PIPE_UNNAMED:
            close_pipe_unnamed(core_h->mech_handle);
            break;
        case IPC_SOCKET_UNIX:
            close_socket_unix(core_h->mech_handle);
            break;
        case IPC_SOCKET_TCP:
            close_socket_tcp(core_h->mech_handle);
            break;
        default:
            break; // No action for invalid mechanism
    }
    free(core_h);
}

// Mutex implementation (process-shared via shared memory)
typedef struct {
    pthread_mutex_t *mutex;
    void *shm;
    size_t shm_size;
} IPC_Mutex_Internal;

IPC_Mutex* ipc_mutex_create(void) {
    IPC_Mutex_Internal *mux = malloc(sizeof(IPC_Mutex_Internal));
    if (!mux) return NULL;

    // Use unique shm name to avoid conflicts
    int fd = shm_open("/ipc_mutex_XXXXXX", O_CREAT | O_RDWR, 0600);
    if (fd == -1) {
        free(mux);
        return NULL;
    }
    mux->shm_size = sizeof(pthread_mutex_t);
    if (ftruncate(fd, mux->shm_size) == -1) {
        close(fd);
        shm_unlink("/ipc_mutex_XXXXXX");
        free(mux);
        return NULL;
    }
    mux->shm = mmap(NULL, mux->shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if (mux->shm == MAP_FAILED) {
        shm_unlink("/ipc_mutex_XXXXXX");
        free(mux);
        return NULL;
    }

    mux->mutex = (pthread_mutex_t *)mux->shm;
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(mux->mutex, &attr);
    pthread_mutexattr_destroy(&attr);

    return (IPC_Mutex *)mux;
}

void ipc_mutex_lock(IPC_Mutex *mux) {
    if (mux) pthread_mutex_lock(((IPC_Mutex_Internal *)mux)->mutex);
}

void ipc_mutex_unlock(IPC_Mutex *mux) {
    if (mux) pthread_mutex_unlock(((IPC_Mutex_Internal *)mux)->mutex);
}

void ipc_mutex_destroy(IPC_Mutex *mux) {
    if (!mux) return;
    IPC_Mutex_Internal *m = (IPC_Mutex_Internal *)mux;
    pthread_mutex_destroy(m->mutex);
    munmap(m->shm, m->shm_size);
    shm_unlink("/ipc_mutex_XXXXXX");
    free(m);
}

// Semaphore implementation (POSIX, process-shared)
typedef struct {
    sem_t *sem;
    char *name;
} IPC_Sem_Internal;

IPC_Sem* ipc_sem_create(int initial_value) {
    IPC_Sem_Internal *sem = malloc(sizeof(IPC_Sem_Internal));
    if (!sem) return NULL;

    sem->name = strdup("/ipc_sem_XXXXXX");
    sem->sem = sem_open(sem->name, O_CREAT, 0600, initial_value);
    if (sem->sem == SEM_FAILED) {
        free(sem->name);
        free(sem);
        return NULL;
    }
    return (IPC_Sem *)sem;
}

void ipc_sem_wait(IPC_Sem *sem) {
    if (sem) sem_wait(((IPC_Sem_Internal *)sem)->sem);
}

void ipc_sem_post(IPC_Sem *sem) {
    if (sem) sem_post(((IPC_Sem_Internal *)sem)->sem);
}

void ipc_sem_destroy(IPC_Sem *sem) {
    if (!sem) return;
    IPC_Sem_Internal *s = (IPC_Sem_Internal *)sem;
    sem_close(s->sem);
    sem_unlink(s->name);
    free(s->name);
    free(s);
}
