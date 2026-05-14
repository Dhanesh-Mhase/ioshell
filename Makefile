CC     = gcc
CFLAGS = -Wall -Wextra -g
LIBS   = -lreadline

TARGET = ioshell
SRCS   = main.c parser.c executor.c builtins.c io.c jobs.c

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) $(LIBS)

clean:
	rm -f $(TARGET)
