# LPB3568 Modbus RTU Master

LPB3568 runs as the Modbus RTU master. `Stm32_modbus` runs as the slave.

The tool opens a serial port and provides an interactive menu:

- select STM32 slave id
- set LED color
- set PWM duty
- collect LED/PWM/heartbeat registers
- read one holding register
- write one holding register

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
./lpb3568_modbus /dev/ttyS7 9600 1
```

Arguments:

- `/dev/ttyS7`: LPB3568 serial device
- `9600`: baudrate
- `1`: default STM32 slave id

## STM32 Holding Registers

| Address | Meaning | Range |
| --- | --- | --- |
| 0 | LED mode | 0 off, 1 red, 2 green, 3 blue, 4 yellow, 5 purple, 6 cyan, 7 white |
| 1 | PWM duty | 0..1000 |
| 2 | Heartbeat | increments on STM32 every second |

## Notes

The collect command reads registers `0`, `1`, and `2` one by one. This matches the current STM32 slave behavior observed on the board.

Set `MODBUS_DEBUG=1` to print TX/RX frames:

```sh
MODBUS_DEBUG=1 ./lpb3568_modbus /dev/ttyS7 9600 1
```
