#ifndef IPC_H
#define IPC_H

#include <stdint.h>
#include <stddef.h>  // For size_t

typedef enum {
    IPC_SHM_MUTEX,
    IPC_MQ_POSIX,
    IPC_MQ_SYSV,
    IPC_PIPE_UNNAMED,
    IPC_PIPE_NAMED,
    IPC_SOCKET_UNIX,
    IPC_SOCKET_TCP
} IPC_Mechanism;

typedef struct {
    IPC_Mechanism mech;
    const char *name;  // For named resources
    uint32_t size;     // For shm/mq size
    int port;          // For TCP sockets
} IPC_Config;

typedef void* IPC_Handle;

IPC_Handle ipc_init(const IPC_Config *config);
int ipc_send(IPC_Handle handle, const void *data, size_t len);
int ipc_recv(IPC_Handle handle, void *buf, size_t len);
void ipc_close(IPC_Handle handle);

typedef struct {} IPC_Mutex;
IPC_Mutex* ipc_mutex_create(void);
void ipc_mutex_lock(IPC_Mutex *mux);
void ipc_mutex_unlock(IPC_Mutex *mux);
void ipc_mutex_destroy(IPC_Mutex *mux);

typedef struct {} IPC_Sem;
IPC_Sem* ipc_sem_create(int initial_value);
void ipc_sem_wait(IPC_Sem *sem);
void ipc_sem_post(IPC_Sem *sem);
void ipc_sem_destroy(IPC_Sem *sem);

#endif
