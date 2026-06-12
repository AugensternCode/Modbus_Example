#include "modbus_master.h"

#include "modbus_crc.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FUNC_READ_HOLDING  0x03U
#define FUNC_WRITE_SINGLE  0x06U

#define MODBUS_RESP_TIMEOUT_MS  500
#define MODBUS_GAP_TIMEOUT_MS   20

static void drain_input(modbus_master_t *ctx)
{
    uint8_t byte;
    int timeout_ms = MODBUS_GAP_TIMEOUT_MS;

    for (unsigned int i = 0U; i < MODBUS_MAX_FRAME; i++) {
        int ret = ctx->recv(&byte, timeout_ms, ctx->io_user);
        if (ret != 1) {
            break;
        }
        timeout_ms = 2;
    }
}

static uint16_t read_u16_be(const uint8_t *p)
{
    return (uint16_t)(((uint16_t)p[0] << 8U) | p[1]);
}

static void write_u16_be(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)(value >> 8U);
    p[1] = (uint8_t)(value & 0x00FFU);
}

static size_t append_crc(uint8_t *frame, size_t len)
{
    uint16_t crc = modbus_crc16(frame, len);

    frame[len] = (uint8_t)(crc & 0x00FFU);
    frame[len + 1U] = (uint8_t)(crc >> 8U);
    return len + 2U;
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

static int fail_frame(const char *reason, const uint8_t *buf, size_t len)
{
    fprintf(stderr, "%s", reason);
    if ((buf != NULL) && (len > 0U)) {
        fprintf(stderr, " frame:");
        for (size_t i = 0U; i < len; i++) {
            fprintf(stderr, " %02X", buf[i]);
        }
    }
    fprintf(stderr, "\n");
    return -EBADMSG;
}

static int read_response(modbus_master_t *ctx, const uint8_t *request,
                         size_t request_len, uint8_t expected_func,
                         size_t expected_len, int skip_echo,
                         uint8_t *buf, size_t cap)
{
    uint8_t echo_prefix[MODBUS_MAX_FRAME];
    size_t echo_len = 0U;
    size_t len = 0U;
    size_t target_len = 0U;
    int timeout_ms = MODBUS_RESP_TIMEOUT_MS;

    while (len < cap) {
        uint8_t byte;
        int ret = ctx->recv(&byte, timeout_ms, ctx->io_user);

        if (ret == 0) {
            if (len == 0U) {
                return -ETIMEDOUT;
            }
            return fail_frame("RX timeout before complete response", buf, len);
        }
        if (ret < 0) {
            return ret;
        }
        timeout_ms = MODBUS_GAP_TIMEOUT_MS;

        if ((skip_echo != 0) && (len == 0U)) {
            if ((echo_len < request_len) && (byte == request[echo_len])) {
                echo_prefix[echo_len] = byte;
                echo_len++;
                if (echo_len == request_len) {
                    echo_len = 0U;
                    timeout_ms = MODBUS_RESP_TIMEOUT_MS;
                }
                continue;
            }

            if (echo_len > 0U) {
                if ((echo_len + 1U) > cap) {
                    return -EMSGSIZE;
                }
                memcpy(buf, echo_prefix, echo_len);
                len = echo_len;
                echo_len = 0U;
            }
        }

        buf[len++] = byte;

        if ((target_len == 0U) && (len >= 2U)) {
            if (buf[1] == (uint8_t)(expected_func | 0x80U)) {
                target_len = 5U;
            } else if (buf[1] == expected_func) {
                target_len = expected_len;
            } else {
                return fail_frame("RX unexpected function", buf, len);
            }
        }

        if ((target_len > 0U) && (len == target_len)) {
            break;
        }
    }

    if ((target_len == 0U) || (len != target_len) || (len < 4U)) {
        return fail_frame("RX bad response length", buf, len);
    }

    {
        uint16_t crc_calc = modbus_crc16(buf, len - 2U);
        uint16_t crc_recv = (uint16_t)buf[len - 2U] |
                            ((uint16_t)buf[len - 1U] << 8U);
        if (crc_calc != crc_recv) {
            return fail_frame("RX bad crc", buf, len);
        }
    }

    if (buf[0] != ctx->slave_id) {
        return fail_frame("RX wrong slave id", buf, len);
    }

    debug_frame("RX:", buf, len);
    return (int)len;
}

void modbus_master_init(modbus_master_t *ctx, uint8_t slave_id,
                        modbus_io_send_fn send,
                        modbus_io_recv_fn recv,
                        void *io_user)
{
    ctx->slave_id = slave_id;
    ctx->send = send;
    ctx->recv = recv;
    ctx->io_user = io_user;
}

int modbus_read_holding(modbus_master_t *ctx, uint16_t addr,
                        uint16_t count, uint16_t *values)
{
    uint8_t req[8];
    uint8_t resp[MODBUS_MAX_FRAME];
    int len;

    if ((count == 0U) || (count > 125U)) {
        return -EINVAL;
    }

    req[0] = ctx->slave_id;
    req[1] = FUNC_READ_HOLDING;
    write_u16_be(&req[2], addr);
    write_u16_be(&req[4], count);

    size_t req_len = append_crc(req, 6U);

    drain_input(ctx);

    debug_frame("TX:", req, req_len);

    if (ctx->send(req, req_len, ctx->io_user) != 0) {
        return -EIO;
    }

    len = read_response(ctx, req, req_len, FUNC_READ_HOLDING,
                        (size_t)(5U + count * 2U), 1, resp, sizeof(resp));
    if (len < 0) {
        return len;
    }

    if (resp[1] == (uint8_t)(FUNC_READ_HOLDING | 0x80U)) {
        return -resp[2];
    }

    if ((resp[1] != FUNC_READ_HOLDING) ||
        (resp[2] != (uint8_t)(count * 2U)) ||
        (len != (int)(5U + count * 2U))) {
        return fail_frame("RX read response format mismatch", resp, (size_t)len);
    }

    for (uint16_t i = 0U; i < count; i++) {
        values[i] = read_u16_be(&resp[3U + i * 2U]);
    }

    return 0;
}

int modbus_write_single(modbus_master_t *ctx, uint16_t addr, uint16_t value)
{
    uint8_t req[8];
    uint8_t resp[MODBUS_MAX_FRAME];
    int len;

    req[0] = ctx->slave_id;
    req[1] = FUNC_WRITE_SINGLE;
    write_u16_be(&req[2], addr);
    write_u16_be(&req[4], value);

    size_t req_len = append_crc(req, 6U);

    drain_input(ctx);

    debug_frame("TX:", req, req_len);

    if (ctx->send(req, req_len, ctx->io_user) != 0) {
        return -EIO;
    }

    len = read_response(ctx, req, req_len, FUNC_WRITE_SINGLE,
                        8U, 0, resp, sizeof(resp));
    if (len < 0) {
        return len;
    }

    if (resp[1] == (uint8_t)(FUNC_WRITE_SINGLE | 0x80U)) {
        return -resp[2];
    }

    if ((len != 8) || (memcmp(req, resp, 8U) != 0)) {
        return fail_frame("RX write response mismatch", resp, (size_t)len);
    }

    drain_input(ctx);
    return 0;
}
