#ifndef IPC_H
#define IPC_H

#include <stdint.h>  // For types like uint32_t
#include <stddef.h>  // For size_t

typedef enum {
    IPC_SHM_MUTEX,       // Shared Memory with Mutex
    IPC_MQ_POSIX,        // POSIX Message Queue
    IPC_MQ_SYSV,         // System V Message Queue
    IPC_PIPE_UNNAMED,    // Unnamed Pipe (one-to-one, related processes)
    IPC_PIPE_NAMED,      // Named Pipe (FIFO)
    IPC_SOCKET_UNIX,     // Unix Domain Socket
    IPC_SOCKET_TCP       // TCP/IP Socket
} IPC_Mechanism;

typedef struct {
    IPC_Mechanism mech;
    const char *name;    // For named resources (e.g., shm name, FIFO path)
    uint32_t size;       // For shm or mq max size
    int port;            // For TCP sockets
    // Add more as needed, e.g., IP addr for TCP
} IPC_Config;

typedef void* IPC_Handle;  // Opaque handle

// Initialization (runtime config via IPC_Config)
IPC_Handle ipc_init(const IPC_Config *config);

// Send data (supports one-to-one/many based on mech)
int ipc_send(IPC_Handle handle, const void *data, size_t len);

// Receive data (blocking by default; add non-block flag if needed)
int ipc_recv(IPC_Handle handle, void *buf, size_t len);

// Close and cleanup
void ipc_close(IPC_Handle handle);

// Built-in Sync APIs (wrappers for pthread mutex/sem; can be POSIX sem for processes)
typedef struct { /* Internal */ } IPC_Mutex;
IPC_Mutex* ipc_mutex_create(void);
void ipc_mutex_lock(IPC_Mutex *mux);
void ipc_mutex_unlock(IPC_Mutex *mux);
void ipc_mutex_destroy(IPC_Mutex *mux);

typedef struct { /* Internal */ } IPC_Sem;
IPC_Sem* ipc_sem_create(int initial_value);
void ipc_sem_wait(IPC_Sem *sem);
void ipc_sem_post(IPC_Sem *sem);
void ipc_sem_destroy(IPC_Sem *sem);

#endif
