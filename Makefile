CFLAGS=-std=gnu99 -g -Wall
TARGET=simple_echo thread_echo pre_thread_echo

all: $(TARGET)

simple_echo: simple_echo.c
	$(CC) $(CFLAGS) -o bin/$@ $<

thread_echo: thread_echo.c
	$(CC) $(CFLAGS) -o bin/$@ $<

pre_thread_echo: pre_thread_echo.c
	$(CC) $(CFLAGS) -o bin/$@ $<

clean:
	rm -f bin/*.o

.PHONY : all clean
