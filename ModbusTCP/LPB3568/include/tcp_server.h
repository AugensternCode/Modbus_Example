#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <stddef.h>
#include <stdint.h>

int tcp_listen_on(const char *port);
int tcp_write_all(int fd, const uint8_t *data, size_t len);
int tcp_read_exact_timeout(int fd, uint8_t *data, size_t len, int timeout_ms);

#endif
