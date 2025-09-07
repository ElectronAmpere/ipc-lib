CC = clang
CFLAGS = -Wall -Wextra -O2 -fPIC -Iinclude
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    LDFLAGS = -shared -lpthread -lrt
else
    LDFLAGS = -shared -lpthread
endif

SRCS = src/ipc_core.c src/shm_mutex.c src/msg_queue.c src/pipes.c src/sockets.c
OBJS = $(SRCS:.c=.o)

libipc.so: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) libipc.so
