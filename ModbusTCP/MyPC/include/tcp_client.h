#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include <stddef.h>
#include <stdint.h>

#ifdef _WIN32
#include <winsock2.h>
typedef SOCKET tcp_socket_t;
#else
typedef int tcp_socket_t;
#endif

int tcp_client_startup(void);
void tcp_client_cleanup(void);
void tcp_close(tcp_socket_t fd);
tcp_socket_t tcp_invalid_socket(void);
int tcp_socket_valid(tcp_socket_t fd);
tcp_socket_t tcp_connect_to(const char *host, const char *port, int timeout_ms);
int tcp_write_all(tcp_socket_t fd, const uint8_t *data, size_t len);
int tcp_read_exact_timeout(tcp_socket_t fd, uint8_t *data, size_t len,
                           int timeout_ms);

#endif
