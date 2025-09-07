#include "ipc.h"
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#ifdef __linux__
#include <mqueue.h>     // For POSIX message queues
#include <sys/msg.h>    // For System V message queues
#endif

// POSIX Message Queue Handle
typedef struct {
#ifdef __linux__
    mqd_t mq;
#else
    void *dummy; // Placeholder for macOS
#endif
} MqPosixHandle;

// System V Message Queue Handle
typedef struct {
#ifdef __linux__
    int msqid;
#else
    void *dummy; // Placeholder for macOS
#endif
} MqSysvHandle;

#ifdef __linux__
// POSIX Message Queue Implementation
IPC_Handle init_mq_posix(const IPC_Config *config) {
    struct mq_attr attr = {
        .mq_maxmsg = 10,          // Max number of messages
        .mq_msgsize = config->size // Max message size
    };
    mqd_t mq = mq_open(config->name, O_CREAT | O_RDWR, 0666, &attr);
    if (mq == (mqd_t)-1) {
        return NULL;
    }

    MqPosixHandle *h = malloc(sizeof(MqPosixHandle));
    if (!h) {
        mq_close(mq);
        mq_unlink(config->name);
        return NULL;
    }
    h->mq = mq;
    return (IPC_Handle)h;
}

int ipc_send_mq_posix(IPC_Handle handle, const void *data, size_t len) {
    MqPosixHandle *h = (MqPosixHandle *)handle;
    if (!h || !data || len == 0) {
        errno = EINVAL;
        return -1;
    }
    return mq_send(h->mq, data, len, 0); // Priority 0
}

int ipc_recv_mq_posix(IPC_Handle handle, void *buf, size_t len) {
    MqPosixHandle *h = (MqPosixHandle *)handle;
    if (!h || !buf || len == 0) {
        errno = EINVAL;
        return -1;
    }
    return mq_receive(h->mq, buf, len, NULL);
}

void close_mq_posix(IPC_Handle handle) {
    MqPosixHandle *h = (MqPosixHandle *)handle;
    if (!h) return;
    mq_close(h->mq);
    mq_unlink(((IPC_Config *)/*from init*/)->name); // Note: Requires config; see below
    free(h);
}

// System V Message Queue Implementation
typedef struct {
    long mtype; // Message type (required by System V)
    char mtext[1]; // Flexible array for data
} SysvMsg;

IPC_Handle init_mq_sysv(const IPC_Config *config) {
    key_t key = ftok(config->name, 'a');
    if (key == -1) {
        return NULL;
    }
    int msqid = msgget(key, IPC_CREAT | 0666);
    if (msqid == -1) {
        return NULL;
    }

    MqSysvHandle *h = malloc(sizeof(MqSysvHandle));
    if (!h) {
        msgctl(msqid, IPC_RMID, NULL);
        return NULL;
    }
    h->msqid = msqid;
    return (IPC_Handle)h;
}

int ipc_send_mq_sysv(IPC_Handle handle, const void *data, size_t len) {
    MqSysvHandle *h = (MqSysvHandle *)handle;
    if (!h || !data || len == 0) {
        errno = EINVAL;
        return -1;
    }
    SysvMsg *msg = malloc(sizeof(SysvMsg) + len);
    if (!msg) {
        errno = ENOMEM;
        return -1;
    }
    msg->mtype = 1; // Default message type
    memcpy(msg->mtext, data, len);
    int ret = msgsnd(h->msqid, msg, len, 0);
    free(msg);
    return ret;
}

int ipc_recv_mq_sysv(IPC_Handle handle, void *buf, size_t len) {
    MqSysvHandle *h = (MqSysvHandle *)handle;
    if (!h || !buf || len == 0) {
        errno = EINVAL;
        return -1;
    }
    SysvMsg *msg = malloc(sizeof(SysvMsg) + len);
    if (!msg) {
        errno = ENOMEM;
        return -1;
    }
    int ret = msgrcv(h->msqid, msg, len, 0, 0);
    if (ret != -1) {
        memcpy(buf, msg->mtext, ret);
    }
    free(msg);
    return ret;
}

void close_mq_sysv(IPC_Handle handle) {
    MqSysvHandle *h = (MqSysvHandle *)handle;
    if (!h) return;
    msgctl(h->msqid, IPC_RMID, NULL);
    free(h);
}

#else // macOS or other non-Linux platforms
IPC_Handle init_mq_posix(const IPC_Config *config) {
    errno = ENOTSUP;
    return NULL;
}

int ipc_send_mq_posix(IPC_Handle handle, const void *data, size_t len) {
    errno = ENOTSUP;
    return -1;
}

int ipc_recv_mq_posix(IPC_Handle handle, void *buf, size_t len) {
    errno = ENOTSUP;
    return -1;
}

void close_mq_posix(IPC_Handle handle) {
    // No-op
}

IPC_Handle init_mq_sysv(const IPC_Config *config) {
    errno = ENOTSUP;
    return NULL;
}

int ipc_send_mq_sysv(IPC_Handle handle, const void *data, size_t len) {
    errno = ENOTSUP;
    return -1;
}

int ipc_recv_mq_sysv(IPC_Handle handle, void *buf, size_t len) {
    errno = ENOTSUP;
    return -1;
}

void close_mq_sysv(IPC_Handle handle) {
    // No-op
}
#endif
