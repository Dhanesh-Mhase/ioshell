CC     = gcc
CFLAGS = -Wall -Wextra -g

TARGET = ioshell
SRCS   = main.c parser.c executor.c builtins.c io.c jobs.c

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET)
