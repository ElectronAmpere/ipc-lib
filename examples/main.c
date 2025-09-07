#include "ipc.h"
#include <stdio.h>

int main() {
    IPC_Config cfg = {
        .mech = IPC_SHM_MUTEX,
        .name = "/myshm",
        .size = 1024
    };
    IPC_Handle h = ipc_init(&cfg);
    if (!h) {
        perror("ipc_init");
        return 1;
    }

    char data[] = "Hello, IPC!";
    if (ipc_send(h, data, sizeof(data)) == -1) {
        perror("ipc_send");
    }

    char buf[1024];
    if (ipc_recv(h, buf, sizeof(buf)) == -1) {
        perror("ipc_recv");
    } else {
        printf("Received: %s\n", buf);
    }

    IPC_Mutex *mux = ipc_mutex_create();
    ipc_mutex_lock(mux);
    // Critical section
    ipc_mutex_unlock(mux);
    ipc_mutex_destroy(mux);

    ipc_close(h);
    return 0;
}
