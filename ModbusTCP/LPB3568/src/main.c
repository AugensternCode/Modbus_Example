#include "modbus_master.h"
#include "serial_port.h"
#include "tcp_server.h"

#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MODBUS_TCP_HEADER_LEN       7U
#define MODBUS_TCP_MAX_ADU          260U
#define MODBUS_PROTO_ID             0x0000U
#define MODBUS_TCP_IDLE_TIMEOUT_MS  1000

#define FUNC_READ_HOLDING           0x03U
#define FUNC_WRITE_SINGLE           0x06U

#define EX_ILLEGAL_FUNCTION         0x01U
#define EX_ILLEGAL_ADDRESS          0x02U
#define EX_ILLEGAL_VALUE            0x03U
#define EX_GATEWAY_TARGET_FAILED    0x0BU

static volatile sig_atomic_t running = 1;

typedef struct {
    int serial_fd;
    modbus_master_t rtu;
} gateway_t;

static uint16_t read_u16_be(const uint8_t *p)
{
    return (uint16_t)(((uint16_t)p[0] << 8U) | p[1]);
}

static void write_u16_be(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)(value >> 8U);
    p[1] = (uint8_t)(value & 0x00FFU);
}

static void on_signal(int signo)
{
    (void)signo;
    running = 0;
}

static int send_serial(const uint8_t *data, size_t len, void *user)
{
    return serial_write_all(*(int *)user, data, len);
}

static int recv_serial(uint8_t *byte, int timeout_ms, void *user)
{
    return serial_read_byte_timeout(*(int *)user, byte, timeout_ms);
}

static void usage(const char *name)
{
    fprintf(stderr,
            "Usage: %s <serial-dev> [baudrate] [tcp-port]\n",
            name);
    fprintf(stderr, "Example: %s /dev/ttyS7 9600 502\n", name);
    fprintf(stderr,
            "TCP unit id is forwarded as the STM32 RTU slave id.\n");
}

static void debug_frame(const char *tag, const uint8_t *buf, size_t len)
{
    if (getenv("MODBUS_DEBUG") == NULL) {
        return;
    }

    fprintf(stderr, "%s", tag);
    for (size_t i = 0U; i < len; i++) {
        fprintf(stderr, " %02X", buf[i]);
    }
    fprintf(stderr, "\n");
}

static uint8_t gateway_exception_from_ret(int ret)
{
    if ((ret <= -1) && (ret >= -4)) {
        return (uint8_t)(-ret);
    }

    return EX_GATEWAY_TARGET_FAILED;
}

static int send_tcp_response(int fd, const uint8_t *req_hdr,
                             const uint8_t *pdu, uint16_t pdu_len)
{
    uint8_t adu[MODBUS_TCP_MAX_ADU];
    size_t adu_len = MODBUS_TCP_HEADER_LEN + pdu_len;

    if (adu_len > sizeof(adu)) {
        return -1;
    }

    memcpy(adu, req_hdr, MODBUS_TCP_HEADER_LEN);
    write_u16_be(&adu[4], (uint16_t)(1U + pdu_len));
    memcpy(&adu[MODBUS_TCP_HEADER_LEN], pdu, pdu_len);

    debug_frame("TCP TX:", adu, adu_len);
    return tcp_write_all(fd, adu, adu_len);
}

static int send_exception(int fd, const uint8_t *req_hdr, uint8_t func,
                          uint8_t code)
{
    uint8_t pdu[2];

    pdu[0] = (uint8_t)(func | 0x80U);
    pdu[1] = code;
    return send_tcp_response(fd, req_hdr, pdu, sizeof(pdu));
}

static int handle_read_holding(int fd, gateway_t *gw, const uint8_t *hdr,
                               const uint8_t *pdu, uint16_t pdu_len)
{
    uint16_t addr;
    uint16_t count;
    uint16_t values[125];
    uint8_t resp[252];
    int ret;

    if (pdu_len != 5U) {
        return send_exception(fd, hdr, pdu[0], EX_ILLEGAL_VALUE);
    }

    addr = read_u16_be(&pdu[1]);
    count = read_u16_be(&pdu[3]);
    if ((count == 0U) || (count > 125U) || (hdr[6] == 0U)) {
        return send_exception(fd, hdr, pdu[0], EX_ILLEGAL_VALUE);
    }

    gw->rtu.slave_id = hdr[6];
    ret = modbus_read_holding(&gw->rtu, addr, count, values);
    if (ret != 0) {
        fprintf(stderr, "RTU read failed: unit=%u addr=%u count=%u ret=%d\n",
                hdr[6], addr, count, ret);
        return send_exception(fd, hdr, pdu[0], gateway_exception_from_ret(ret));
    }

    resp[0] = FUNC_READ_HOLDING;
    resp[1] = (uint8_t)(count * 2U);
    for (uint16_t i = 0U; i < count; i++) {
        write_u16_be(&resp[2U + i * 2U], values[i]);
    }

    return send_tcp_response(fd, hdr, resp, (uint16_t)(2U + count * 2U));
}

static int handle_write_single(int fd, gateway_t *gw, const uint8_t *hdr,
                               const uint8_t *pdu, uint16_t pdu_len)
{
    uint16_t addr;
    uint16_t value;
    int ret;

    if (pdu_len != 5U) {
        return send_exception(fd, hdr, pdu[0], EX_ILLEGAL_VALUE);
    }
    if (hdr[6] == 0U) {
        return send_exception(fd, hdr, pdu[0], EX_ILLEGAL_VALUE);
    }

    addr = read_u16_be(&pdu[1]);
    value = read_u16_be(&pdu[3]);

    gw->rtu.slave_id = hdr[6];
    ret = modbus_write_single(&gw->rtu, addr, value);
    if (ret != 0) {
        fprintf(stderr, "RTU write failed: unit=%u addr=%u value=%u ret=%d\n",
                hdr[6], addr, value, ret);
        return send_exception(fd, hdr, pdu[0], gateway_exception_from_ret(ret));
    }

    return send_tcp_response(fd, hdr, pdu, pdu_len);
}

static int handle_tcp_request(int fd, gateway_t *gw, const uint8_t *hdr,
                              const uint8_t *pdu, uint16_t pdu_len)
{
    if (pdu_len == 0U) {
        return -1;
    }

    switch (pdu[0]) {
    case FUNC_READ_HOLDING:
        return handle_read_holding(fd, gw, hdr, pdu, pdu_len);
    case FUNC_WRITE_SINGLE:
        return handle_write_single(fd, gw, hdr, pdu, pdu_len);
    default:
        return send_exception(fd, hdr, pdu[0], EX_ILLEGAL_FUNCTION);
    }
}

static int handle_client(int fd, gateway_t *gw)
{
    while (running) {
        uint8_t hdr[MODBUS_TCP_HEADER_LEN];
        uint8_t pdu[MODBUS_TCP_MAX_ADU];
        uint16_t proto;
        uint16_t length;
        uint16_t pdu_len;
        int ret;

        ret = tcp_read_exact_timeout(fd, hdr, sizeof(hdr),
                                     MODBUS_TCP_IDLE_TIMEOUT_MS);
        if (ret == 0) {
            continue;
        }
        if (ret < 0) {
            return -1;
        }

        proto = read_u16_be(&hdr[2]);
        length = read_u16_be(&hdr[4]);
        if ((proto != MODBUS_PROTO_ID) || (length < 2U) ||
            (length > (MODBUS_TCP_MAX_ADU - 6U))) {
            fprintf(stderr, "Bad TCP MBAP header\n");
            return -1;
        }

        pdu_len = (uint16_t)(length - 1U);
        ret = tcp_read_exact_timeout(fd, pdu, pdu_len, 1000);
        if (ret <= 0) {
            return -1;
        }

        debug_frame("TCP RX-HDR:", hdr, sizeof(hdr));
        debug_frame("TCP RX-PDU:", pdu, pdu_len);

        if (handle_tcp_request(fd, gw, hdr, pdu, pdu_len) != 0) {
            return -1;
        }
    }

    return 0;
}

int main(int argc, char **argv)
{
    const char *serial_dev;
    const char *tcp_port = "502";
    int baudrate = 9600;
    int listen_fd;
    gateway_t gw;

    if (argc < 2) {
        usage(argv[0]);
        return 2;
    }

    serial_dev = argv[1];
    if (argc >= 3) {
        baudrate = atoi(argv[2]);
    }
    if (argc >= 4) {
        tcp_port = argv[3];
    }

    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);

    gw.serial_fd = serial_open(serial_dev, baudrate);
    if (gw.serial_fd < 0) {
        fprintf(stderr, "open serial %s failed: %s\n", serial_dev,
                strerror(errno));
        return 1;
    }

    modbus_master_init(&gw.rtu, 1U, send_serial, recv_serial, &gw.serial_fd);

    listen_fd = tcp_listen_on(tcp_port);
    if (listen_fd < 0) {
        fprintf(stderr, "listen TCP port %s failed: %s\n", tcp_port,
                strerror(errno));
        close(gw.serial_fd);
        return 1;
    }

    printf("LPB3568 Modbus TCP to RTU gateway\n");
    printf("  TCP listen port : %s\n", tcp_port);
    printf("  RTU serial dev  : %s\n", serial_dev);
    printf("  RTU baudrate    : %d\n", baudrate);
    printf("  TCP unit id maps to STM32 RTU slave id\n");

    while (running) {
        int client_fd = accept(listen_fd, NULL, NULL);

        if (client_fd < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }

        printf("TCP client connected\n");
        handle_client(client_fd, &gw);
        close(client_fd);
        printf("TCP client disconnected\n");
    }

    close(listen_fd);
    close(gw.serial_fd);
    return 0;
}
