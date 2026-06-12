CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -std=c99
CPPFLAGS ?= -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L -Iinclude
LDFLAGS ?=

TARGET := mypc_modbus_tcp_master
ifeq ($(OS),Windows_NT)
TARGET := mypc_modbus_tcp_master.exe
LDFLAGS += -lws2_32
endif
SRCS := \
	src/main.c \
	src/modbus_tcp_master.c \
	src/tcp_client.c
OBJS := $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

clean:
	rm -f $(OBJS) $(TARGET)
