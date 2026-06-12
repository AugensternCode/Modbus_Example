# MyPC Modbus TCP Master

This program runs on your PC. It connects to LPB3568 by Modbus TCP.
LPB3568 then forwards each request to STM32 by Modbus RTU.

## Build

Linux:

```sh
make
```

Windows with MinGW/MSYS2:

```sh
mingw32-make
```

Or compile directly:

```sh
gcc -O2 -Wall -Wextra -std=c99 -Iinclude src/main.c src/modbus_tcp_master.c src/tcp_client.c -lws2_32 -o mypc_modbus_tcp_master.exe
```

## Run

Linux:

```sh
./mypc_modbus_tcp_master <lpb3568-ip> [tcp-port] [stm32-slave-id]
```

Windows:

```sh
mypc_modbus_tcp_master.exe <lpb3568-ip> [tcp-port] [stm32-slave-id]
```

Example:

```sh
./mypc_modbus_tcp_master 192.168.1.30 502 1
```

The `stm32-slave-id` becomes the Modbus TCP unit id. LPB3568 uses that unit id
as the RTU slave id when forwarding the request to STM32.

Set `MODBUS_DEBUG=1` to print TCP frames.
