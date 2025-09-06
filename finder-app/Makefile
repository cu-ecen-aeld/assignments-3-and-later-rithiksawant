# Makefile for writer application

CC ?= gcc
CFLAGS = -Wall -Wextra -std=c99
TARGET = writer
SOURCES = writer.c

ifdef CROSS_COMPILE
    CC = $(CROSS_COMPILE)gcc
endif

# Default target - build the writer application directly from source
$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET)

# Clean target - remove executable
clean:
	rm -f $(TARGET) *.o

# Phony targets
.PHONY: clean
