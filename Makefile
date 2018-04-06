CFLAGS=-std=gnu99 -g -Wall
TARGET=simple_echo

all: $(TARGET)

simple_echo: simple_echo.c
	$(CC) $(CFLAGS) -c -o bin/$@ $<

clean:
	rm -f bin/*.o

.PHONY : all clean
