#ifndef MODBUS_TCP_MASTER_H
#define MODBUS_TCP_MASTER_H

#include <stddef.h>
#include <stdint.h>

#define MODBUS_TCP_MAX_ADU 260U

typedef int (*modbus_tcp_send_fn)(const uint8_t *data, size_t len, void *user);
typedef int (*modbus_tcp_recv_fn)(uint8_t *data, size_t len, int timeout_ms,
                                  void *user);

typedef struct {
    uint8_t unit_id;
    uint16_t transaction_id;
    modbus_tcp_send_fn send;
    modbus_tcp_recv_fn recv;
    void *io_user;
} modbus_tcp_master_t;

void modbus_tcp_master_init(modbus_tcp_master_t *ctx, uint8_t unit_id,
                            modbus_tcp_send_fn send,
                            modbus_tcp_recv_fn recv,
                            void *io_user);
int modbus_tcp_read_holding(modbus_tcp_master_t *ctx, uint16_t addr,
                            uint16_t count, uint16_t *values);
int modbus_tcp_write_single(modbus_tcp_master_t *ctx, uint16_t addr,
                            uint16_t value);

#endif
