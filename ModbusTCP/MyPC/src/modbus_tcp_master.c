#include "modbus_tcp_master.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MODBUS_PROTO_ID        0x0000U
#define MODBUS_TCP_HEADER_LEN  7U
#define MODBUS_TCP_TIMEOUT_MS  1000

#define FUNC_READ_HOLDING      0x03U
#define FUNC_WRITE_SINGLE      0x06U

static uint16_t read_u16_be(const uint8_t *p)
{
    return (uint16_t)(((uint16_t)p[0] << 8U) | p[1]);
}

static void write_u16_be(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)(value >> 8U);
    p[1] = (uint8_t)(value & 0x00FFU);
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

static uint16_t next_transaction_id(modbus_tcp_master_t *ctx)
{
    ctx->transaction_id++;
    if (ctx->transaction_id == 0U) {
        ctx->transaction_id = 1U;
    }
    return ctx->transaction_id;
}

static int transact(modbus_tcp_master_t *ctx, uint8_t func,
                    const uint8_t *payload, size_t payload_len,
                    uint8_t *resp_pdu, size_t *resp_pdu_len)
{
    uint8_t req[MODBUS_TCP_MAX_ADU];
    uint8_t hdr[MODBUS_TCP_HEADER_LEN];
    uint16_t tid = next_transaction_id(ctx);
    uint16_t length;
    int ret;

    if ((payload_len + 8U) > sizeof(req)) {
        return -EMSGSIZE;
    }

    write_u16_be(&req[0], tid);
    write_u16_be(&req[2], MODBUS_PROTO_ID);
    write_u16_be(&req[4], (uint16_t)(2U + payload_len));
    req[6] = ctx->unit_id;
    req[7] = func;
    memcpy(&req[8], payload, payload_len);

    debug_frame("TX:", req, 8U + payload_len);
    if (ctx->send(req, 8U + payload_len, ctx->io_user) != 0) {
        return -EIO;
    }

    ret = ctx->recv(hdr, sizeof(hdr), MODBUS_TCP_TIMEOUT_MS, ctx->io_user);
    if (ret == 0) {
        return -ETIMEDOUT;
    }
    if (ret < 0) {
        return -EIO;
    }

    length = read_u16_be(&hdr[4]);
    if ((read_u16_be(&hdr[0]) != tid) || (read_u16_be(&hdr[2]) != 0U) ||
        (length < 2U) || (length > (MODBUS_TCP_MAX_ADU - 6U)) ||
        (hdr[6] != ctx->unit_id)) {
        return fail_frame("RX bad MBAP header", hdr, sizeof(hdr));
    }

    *resp_pdu_len = (size_t)length - 1U;
    ret = ctx->recv(resp_pdu, *resp_pdu_len, MODBUS_TCP_TIMEOUT_MS,
                    ctx->io_user);
    if (ret == 0) {
        return -ETIMEDOUT;
    }
    if (ret < 0) {
        return -EIO;
    }

    if (resp_pdu[0] == (uint8_t)(func | 0x80U)) {
        if (*resp_pdu_len < 2U) {
            return fail_frame("RX bad exception PDU", resp_pdu, *resp_pdu_len);
        }
        return -resp_pdu[1];
    }
    if (resp_pdu[0] != func) {
        return fail_frame("RX unexpected function", resp_pdu, *resp_pdu_len);
    }

    debug_frame("RX-PDU:", resp_pdu, *resp_pdu_len);
    return 0;
}

void modbus_tcp_master_init(modbus_tcp_master_t *ctx, uint8_t unit_id,
                            modbus_tcp_send_fn send,
                            modbus_tcp_recv_fn recv,
                            void *io_user)
{
    ctx->unit_id = unit_id;
    ctx->transaction_id = 0U;
    ctx->send = send;
    ctx->recv = recv;
    ctx->io_user = io_user;
}

int modbus_tcp_read_holding(modbus_tcp_master_t *ctx, uint16_t addr,
                            uint16_t count, uint16_t *values)
{
    uint8_t payload[4];
    uint8_t resp[MODBUS_TCP_MAX_ADU];
    size_t resp_len;
    int ret;

    if ((count == 0U) || (count > 125U)) {
        return -EINVAL;
    }

    write_u16_be(&payload[0], addr);
    write_u16_be(&payload[2], count);
    ret = transact(ctx, FUNC_READ_HOLDING, payload, sizeof(payload), resp,
                   &resp_len);
    if (ret != 0) {
        return ret;
    }

    if ((resp_len != (size_t)(2U + count * 2U)) ||
        (resp[1] != (uint8_t)(count * 2U))) {
        return fail_frame("RX read response format mismatch", resp, resp_len);
    }

    for (uint16_t i = 0U; i < count; i++) {
        values[i] = read_u16_be(&resp[2U + i * 2U]);
    }

    return 0;
}

int modbus_tcp_write_single(modbus_tcp_master_t *ctx, uint16_t addr,
                            uint16_t value)
{
    uint8_t payload[4];
    uint8_t resp[MODBUS_TCP_MAX_ADU];
    size_t resp_len;
    int ret;

    write_u16_be(&payload[0], addr);
    write_u16_be(&payload[2], value);
    ret = transact(ctx, FUNC_WRITE_SINGLE, payload, sizeof(payload), resp,
                   &resp_len);
    if (ret != 0) {
        return ret;
    }

    if ((resp_len != 5U) || (memcmp(&resp[1], payload, sizeof(payload)) != 0)) {
        return fail_frame("RX write response mismatch", resp, resp_len);
    }

    return 0;
}
