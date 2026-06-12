# Modbus TCP Gateway Layout

Target chain:

```text
MyPC (Modbus TCP master)
  -> Ethernet/Wi-Fi
LPB3568 (Modbus TCP server + Modbus RTU master gateway)
  -> RS485
STM32 (existing Modbus RTU slave)
```

Folders:

- `MyPC`: PC-side Modbus TCP master program.
- `LPB3568_TCP`: board-side Modbus TCP to RTU gateway.

The STM32 register map stays the same as your existing RTU project:

| Address | Meaning | Range |
| --- | --- | --- |
| 0 | LED mode | 0 off, 1 red, 2 green, 3 blue, 4 yellow, 5 purple, 6 cyan, 7 white |
| 1 | PWM duty | 0..1000 |
| 2 | Heartbeat | increments on STM32 every second |

## Typical Run

On LPB3568:

```sh
cd ModbusTCP/LPB3568_TCP
make
sudo ./lpb3568_modbus_tcp_gateway /dev/ttyS7 9600 502
```

On MyPC:

```sh
cd ModbusTCP/MyPC
make
./mypc_modbus_tcp_master <lpb3568-ip> 502 1
```

On Windows with MinGW/MSYS2:

```sh
cd ModbusTCP/MyPC
mingw32-make
mypc_modbus_tcp_master.exe <lpb3568-ip> 502 1
```

The final `1` is the STM32 RTU slave id. It is sent as the Modbus TCP unit id,
and LPB3568 forwards it to the RTU bus.

For a step-by-step Chinese guide, see `使用文档.md`.
