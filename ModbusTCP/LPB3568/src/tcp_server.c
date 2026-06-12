#include "tcp_server.h"

#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static int wait_readable(int fd, int timeout_ms)
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
        ret = select(fd + 1, &fds, NULL, NULL, tvp);
    } while ((ret < 0) && (errno == EINTR));

    return ret;
}

int tcp_listen_on(const char *port)
{
    struct addrinfo hints;
    struct addrinfo *res = NULL;
    struct addrinfo *ai;
    int listen_fd = -1;
    int opt = 1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, port, &hints, &res) != 0) {
        return -1;
    }

    for (ai = res; ai != NULL; ai = ai->ai_next) {
        listen_fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (listen_fd < 0) {
            continue;
        }

        setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        if ((bind(listen_fd, ai->ai_addr, ai->ai_addrlen) == 0) &&
            (listen(listen_fd, 4) == 0)) {
            break;
        }

        close(listen_fd);
        listen_fd = -1;
    }

    freeaddrinfo(res);
    return listen_fd;
}

int tcp_write_all(int fd, const uint8_t *data, size_t len)
{
    size_t sent = 0U;

    while (sent < len) {
        ssize_t ret = send(fd, data + sent, len - sent, 0);

        if (ret < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (ret == 0) {
            return -1;
        }

        sent += (size_t)ret;
    }

    return 0;
}

int tcp_read_exact_timeout(int fd, uint8_t *data, size_t len, int timeout_ms)
{
    size_t got = 0U;

    while (got < len) {
        int ready = wait_readable(fd, timeout_ms);
        ssize_t ret;

        if (ready == 0) {
            return 0;
        }
        if (ready < 0) {
            return -1;
        }

        ret = recv(fd, data + got, len - got, 0);
        if (ret < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (ret == 0) {
            return -1;
        }

        got += (size_t)ret;
    }

    return 1;
}
