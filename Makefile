CFLAGS=-std=gnu99 -g -Wall -pthread
TARGET=simple_echo thread_echo pre_thread_echo fork_echo pre_fork_echo epoll_pre_thread_echo epoll_pre_fork_echo epoll_echo

all: $(TARGET)

simple_echo: simple_echo.c
	$(CC) $(CFLAGS) -o bin/$@ $<

epoll_echo: epoll_echo.c
	$(CC) $(CFLAGS) -o bin/$@ $<

thread_echo: thread_echo.c
	$(CC) $(CFLAGS) -o bin/$@ $<

pre_thread_echo: pre_thread_echo.c
	$(CC) $(CFLAGS) -o bin/$@ $<

epoll_pre_thread_echo: epoll_pre_thread_echo.c
	$(CC) $(CFLAGS) -o bin/$@ $<

epoll_pre_fork_echo: epoll_pre_fork_echo.c
	$(CC) $(CFLAGS) -o bin/$@ $<

fork_echo: fork_echo.c
	$(CC) $(CFLAGS) -o bin/$@ $<

pre_fork_echo: pre_fork_echo.c
	$(CC) $(CFLAGS) -o bin/$@ $<

clean:
	rm -f bin/*.o

.PHONY : all clean
