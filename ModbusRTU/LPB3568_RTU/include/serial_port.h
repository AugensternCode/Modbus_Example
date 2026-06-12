#ifndef SERIAL_PORT_H
#define SERIAL_PORT_H

#include <stddef.h>
#include <stdint.h>

int serial_open(const char *dev, int baudrate);
int serial_read_byte_timeout(int fd, uint8_t *byte, int timeout_ms);
int serial_write_all(int fd, const uint8_t *data, size_t len);

#endif
