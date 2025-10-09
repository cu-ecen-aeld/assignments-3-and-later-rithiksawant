CC := gcc
CROSS_COMPILE ?=
CC := $(CROSS_COMPILE)gcc

CFLAGS := -Wall -Werror -g -O0 -std=gnu99
LDFLAGS := -pthread

TARGET := aesdsocket
SOURCE := aesdsocket.c

# Default target
all: $(TARGET)

# Build the target
$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE) $(LDFLAGS)

# Clean target
clean:
	rm -f $(TARGET)

# Install target (for cross-compilation scenarios)
install: $(TARGET)
	install -m 755 $(TARGET) /usr/bin/

# Phony targets
.PHONY: all clean install default

# Default target alias
default: all
