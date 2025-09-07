#include "../include/ipc.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

int main() {
    // Initialize semaphore for synchronization
    IPC_Sem *sem = ipc_sem_create(0);
    if (!sem) {
        perror("ipc_sem_create");
        return 1;
    }

    // Initialize config with dynamic name
    char name[32];
    snprintf(name, sizeof(name), "/myshm_%d", getpid());
    IPC_Config cfg = {
        .mech = IPC_SHM_MUTEX,
        .name = name,
        .size = 1024,
        .port = 0
    };

    // Debug: Print config values
    printf("Config: mech=%d, name=%s, size=%u, port=%d\n",
           cfg.mech, cfg.name ? cfg.name : "NULL", cfg.size, cfg.port);

    // Fork to create sender and receiver
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        ipc_sem_destroy(sem);
        return 1;
    }

    if (pid == 0) { // Child (receiver)
        // Wait for parent to write
        ipc_sem_wait(sem);

        IPC_Handle h = ipc_init(&cfg);
        if (!h) {
            perror("ipc_init");
            ipc_sem_destroy(sem);
            return 1;
        }

        char buf[1024] = {0};
        int ret = ipc_recv(h, buf, sizeof(buf));
        if (ret == -1) {
            perror("ipc_recv");
            ipc_close(h);
            ipc_sem_destroy(sem);
            return 1;
        }
        printf("Child received: %s\n", buf);
        ipc_close(h);
        ipc_sem_destroy(sem);
        return 0;
    }

    // Parent (sender)
    IPC_Handle h = ipc_init(&cfg);
    if (!h) {
        perror("ipc_init");
        ipc_sem_destroy(sem);
        return 1;
    }

    const char *data = "Hello, Shared Memory!";
    if (ipc_send(h, data, strlen(data) + 1) == -1) {
        perror("ipc_send");
        ipc_close(h);
        ipc_sem_destroy(sem);
        return 1;
    }

    // Signal child to read
    ipc_sem_post(sem);

    // Wait for child to finish
    sleep(1);
    ipc_close(h);
    ipc_sem_destroy(sem);
    return 0;
}
