#ifndef MODBUS_MASTER_H
#define MODBUS_MASTER_H

#include <stddef.h>
#include <stdint.h>

#define MODBUS_MAX_FRAME 256U

typedef int (*modbus_io_send_fn)(const uint8_t *data, size_t len, void *user);
typedef int (*modbus_io_recv_fn)(uint8_t *byte, int timeout_ms, void *user);

typedef struct {
    uint8_t slave_id;
    modbus_io_send_fn send;
    modbus_io_recv_fn recv;
    void *io_user;
} modbus_master_t;

void modbus_master_init(modbus_master_t *ctx, uint8_t slave_id,
                        modbus_io_send_fn send,
                        modbus_io_recv_fn recv,
                        void *io_user);
int modbus_read_holding(modbus_master_t *ctx, uint16_t addr,
                        uint16_t count, uint16_t *values);
int modbus_write_single(modbus_master_t *ctx, uint16_t addr, uint16_t value);

#endif
