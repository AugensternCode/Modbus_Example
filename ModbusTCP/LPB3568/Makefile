CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -std=c99
CPPFLAGS ?= -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L -Iinclude
LDFLAGS ?=

TARGET := lpb3568_modbus_tcp_gateway
SRCS := \
	src/main.c \
	src/tcp_server.c \
	src/modbus_master.c \
	src/modbus_crc.c \
	src/serial_port.c
OBJS := $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

clean:
	rm -f $(OBJS) $(TARGET)
