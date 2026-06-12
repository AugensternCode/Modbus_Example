# LPB3568 Modbus TCP to RTU Gateway

This program runs on LPB3568.

It listens as a Modbus TCP server/slave. For each TCP request:

1. The TCP unit id is used as the STM32 RTU slave id.
2. The request is forwarded to STM32 through the existing RS485/RTU link.
3. The RTU response is converted back to Modbus TCP and returned to MyPC.

Supported function codes:

- `0x03`: Read Holding Registers
- `0x06`: Write Single Holding Register

## Build

```sh
make
```

Cross compile:

```sh
make CC=aarch64-linux-gnu-gcc
```

## Run

```sh
./lpb3568_modbus_tcp_gateway <serial-dev> [baudrate] [tcp-port]
```

Example:

```sh
sudo ./lpb3568_modbus_tcp_gateway /dev/ttyS7 9600 502
```

If port `502` needs root permission on your Linux system, use `sudo` or choose
a high port such as `1502`.

Set `MODBUS_DEBUG=1` to print TCP and RTU frames.
