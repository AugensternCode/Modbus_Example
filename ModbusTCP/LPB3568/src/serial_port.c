#include "serial_port.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <termios.h>
#include <unistd.h>

static speed_t baud_to_termios(int baudrate)
{
    switch (baudrate) {
    case 1200: return B1200;
    case 2400: return B2400;
    case 4800: return B4800;
    case 9600: return B9600;
    case 19200: return B19200;
    case 38400: return B38400;
    case 57600: return B57600;
    case 115200: return B115200;
    default: return B9600;
    }
}

int serial_open(const char *dev, int baudrate)
{
    int fd;
    struct termios tio;
    speed_t speed;

    fd = open(dev, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        return -1;
    }

    if (tcgetattr(fd, &tio) != 0) {
        close(fd);
        return -1;
    }

    cfmakeraw(&tio);
    tio.c_cflag |= (CLOCAL | CREAD);
    tio.c_cflag &= ~PARENB;
    tio.c_cflag &= ~CSTOPB;
    tio.c_cflag &= ~CSIZE;
    tio.c_cflag |= CS8;
    tio.c_cc[VMIN] = 0;
    tio.c_cc[VTIME] = 0;

    speed = baud_to_termios(baudrate);
    cfsetispeed(&tio, speed);
    cfsetospeed(&tio, speed);

    if (tcsetattr(fd, TCSANOW, &tio) != 0) {
        close(fd);
        return -1;
    }

    tcflush(fd, TCIOFLUSH);
    return fd;
}

int serial_read_byte_timeout(int fd, uint8_t *byte, int timeout_ms)
{
    struct pollfd pfd;
    ssize_t n;
    int ret;

    pfd.fd = fd;
    pfd.events = POLLIN;
    pfd.revents = 0;

    ret = poll(&pfd, 1U, timeout_ms);
    if (ret <= 0) {
        return ret;
    }

    n = read(fd, byte, 1U);
    if (n == 1) {
        return 1;
    }

    if ((n < 0) && ((errno == EAGAIN) || (errno == EWOULDBLOCK))) {
        return 0;
    }

    return -1;
}

int serial_write_all(int fd, const uint8_t *data, size_t len)
{
    size_t sent = 0U;

    while (sent < len) {
        ssize_t n = write(fd, data + sent, len - sent);

        if (n > 0) {
            sent += (size_t)n;
            continue;
        }
        if ((n < 0) && ((errno == EAGAIN) || (errno == EWOULDBLOCK))) {
            continue;
        }
        return -1;
    }

    tcdrain(fd);
    return 0;
}
