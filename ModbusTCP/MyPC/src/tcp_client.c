#include "tcp_client.h"

#include <errno.h>

#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <fcntl.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include <string.h>

int tcp_client_startup(void)
{
#ifdef _WIN32
    WSADATA wsa;
    return WSAStartup(MAKEWORD(2, 2), &wsa) == 0 ? 0 : -1;
#else
    return 0;
#endif
}

void tcp_client_cleanup(void)
{
#ifdef _WIN32
    WSACleanup();
#endif
}

void tcp_close(tcp_socket_t fd)
{
#ifdef _WIN32
    closesocket(fd);
#else
    close(fd);
#endif
}

tcp_socket_t tcp_invalid_socket(void)
{
#ifdef _WIN32
    return INVALID_SOCKET;
#else
    return -1;
#endif
}

int tcp_socket_valid(tcp_socket_t fd)
{
#ifdef _WIN32
    return fd != INVALID_SOCKET;
#else
    return fd >= 0;
#endif
}

static int wait_fd(tcp_socket_t fd, int for_write, int timeout_ms)
{
    fd_set fds;
    struct timeval tv;
    struct timeval *tvp = NULL;
    int ret;

    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    if (timeout_ms >= 0) {
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        tvp = &tv;
    }

    do {
#ifdef _WIN32
        ret = select(0, for_write ? NULL : &fds, for_write ? &fds : NULL,
                     NULL, tvp);
#else
        ret = select(fd + 1, for_write ? NULL : &fds, for_write ? &fds : NULL,
                     NULL, tvp);
#endif
    } while ((ret < 0) && (errno == EINTR));

    return ret;
}

static int set_nonblock(tcp_socket_t fd, int enabled)
{
#ifdef _WIN32
    u_long mode = enabled ? 1UL : 0UL;
    return ioctlsocket(fd, FIONBIO, &mode);
#else
    int flags = fcntl(fd, F_GETFL, 0);

    if (flags < 0) {
        return -1;
    }

    if (enabled) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }

    return fcntl(fd, F_SETFL, flags);
#endif
}

tcp_socket_t tcp_connect_to(const char *host, const char *port, int timeout_ms)
{
    struct addrinfo hints;
    struct addrinfo *res = NULL;
    struct addrinfo *ai;
    tcp_socket_t fd = tcp_invalid_socket();

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port, &hints, &res) != 0) {
        return tcp_invalid_socket();
    }

    for (ai = res; ai != NULL; ai = ai->ai_next) {
        int err = 0;
#ifdef _WIN32
        int err_len = sizeof(err);
#else
        socklen_t err_len = sizeof(err);
#endif

        fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (!tcp_socket_valid(fd)) {
            continue;
        }

        if (set_nonblock(fd, 1) != 0) {
            tcp_close(fd);
            fd = tcp_invalid_socket();
            continue;
        }

        if (connect(fd, ai->ai_addr, ai->ai_addrlen) == 0) {
            set_nonblock(fd, 0);
            break;
        }

        if ((wait_fd(fd, 1, timeout_ms) <= 0) ||
#ifdef _WIN32
            (getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *)&err, &err_len) !=
             0) ||
#else
            (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &err_len) != 0) ||
#endif
            (err != 0)) {
            tcp_close(fd);
            fd = tcp_invalid_socket();
            continue;
        }

        set_nonblock(fd, 0);
        break;
    }

    freeaddrinfo(res);
    return fd;
}

int tcp_write_all(tcp_socket_t fd, const uint8_t *data, size_t len)
{
    size_t sent = 0U;

    while (sent < len) {
#ifdef _WIN32
        int ret = send(fd, (const char *)data + sent, (int)(len - sent), 0);
#else
        ssize_t ret = send(fd, data + sent, len - sent, 0);
#endif

        if (ret < 0) {
#ifndef _WIN32
            if (errno == EINTR) {
                continue;
            }
#endif
            return -1;
        }
        if (ret == 0) {
            return -1;
        }

        sent += (size_t)ret;
    }

    return 0;
}

int tcp_read_exact_timeout(tcp_socket_t fd, uint8_t *data, size_t len,
                           int timeout_ms)
{
    size_t got = 0U;

    while (got < len) {
        int ready = wait_fd(fd, 0, timeout_ms);
        int ret;

        if (ready == 0) {
            return 0;
        }
        if (ready < 0) {
            return -1;
        }

#ifdef _WIN32
        ret = recv(fd, (char *)data + got, (int)(len - got), 0);
#else
        ret = (int)recv(fd, data + got, len - got, 0);
#endif
        if (ret < 0) {
#ifndef _WIN32
            if (errno == EINTR) {
                continue;
            }
#endif
            return -1;
        }
        if (ret == 0) {
            return -1;
        }

        got += (size_t)ret;
    }

    return 1;
}
