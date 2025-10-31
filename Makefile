CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Wpedantic -D_POSIX_C_SOURCE=200809L
LDFLAGS =
TARGET = netstat_monitor
SOURCE = src/netstat_monitor.c

.PHONY: all clean test install

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE) $(LDFLAGS)
	@echo "Build complete: ./$(TARGET)"
	@echo "Run './$(TARGET) --help' for usage information"

clean:
	rm -f $(TARGET)

test: $(TARGET)
	@echo "Testing on loopback interface (lo)..."
	@echo "Running 5 iterations with 1-second interval..."
	./$(TARGET) lo -i 1 -n 5

install: $(TARGET)
	install -m 0755 $(TARGET) /usr/local/bin/

debug: CFLAGS += -g -DDEBUG
debug: clean $(TARGET)
