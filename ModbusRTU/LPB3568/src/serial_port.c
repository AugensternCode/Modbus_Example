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
    //该结构体在windows系统下无法查看，因为poll是Linux系统特有的
    struct pollfd pfd; //监控任务清单
    ssize_t n;
    pfd.fd = fd;  //监控谁？ 这里是串口文件的描述符fd
    pfd.events = POLLIN;  //监控时间：POLLIN代表有数据可读
    pfd.revents = 0;  //结果反馈，0表示还没发生
    int ret = poll(&pfd, 1U, timeout_ms); //执行监控，盯着这一个串口，有数据进来或者
    if (ret <= 0) {   //==0代表超时了，<0代表监控过程出错
        return ret;
    }
    n = read(fd, byte, 1U); //read一般是阻塞的，poll就是为了实现监控超时的
    if (n == 1) { //读取成功
        return 1;
    }
    /*EAGAIN：全称 Error: Try Again（请重试）
    EWOULDBLOCK：全称 Error: Would Block（会阻塞）
    */
    if ((n < 0) && ((errno == EAGAIN) || (errno == EWOULDBLOCK))) {
        return 0; //超时处理
    }
    return -1;  //硬件底层彻底报错）
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
