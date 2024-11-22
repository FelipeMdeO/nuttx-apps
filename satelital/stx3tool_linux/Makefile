# Makefile

CC = gcc
CFLAGS = -Wall -Wextra -O0 -g
TARGET = stx3tool
SRCS = stx3_main.c stx3.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)
