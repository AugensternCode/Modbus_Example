#include "modbus_master.h"
#include "serial_port.h"

#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define STM32_REG_LED_MODE   0U
#define STM32_REG_PWM_DUTY   1U
#define STM32_REG_HEARTBEAT  2U

static volatile sig_atomic_t running = 1;

static int read_one(modbus_master_t *master, uint16_t addr, uint16_t *value);

static const char *led_name(uint16_t mode)
{
    switch (mode) {
    case 0U: return "off";
    case 1U: return "red";
    case 2U: return "green";
    case 3U: return "blue";
    case 4U: return "yellow";
    case 5U: return "purple";
    case 6U: return "cyan";
    case 7U: return "white";
    default: return "unknown";
    }
}

static void on_signal(int signo)
{
    (void)signo;
    running = 0;
}

static int send_serial(const uint8_t *data, size_t len, void *user)
{
    int fd = *(int *)user;
    return serial_write_all(fd, data, len);
}

static int recv_serial(uint8_t *byte, int timeout_ms, void *user)
{
    int fd = *(int *)user;
    return serial_read_byte_timeout(fd, byte, timeout_ms);
}

static void usage(const char *name)
{
    fprintf(stderr, "Usage: %s <serial-dev> [baudrate] [stm32-slave-id]\n", name);
    fprintf(stderr, "Example: %s /dev/ttyS7 9600 1\n", name);
}

static int read_line(char *buf, size_t size)
{
    size_t len;

    if (fgets(buf, (int)size, stdin) == NULL) {
        return -1;
    }

    len = strlen(buf);
    if ((len > 0U) && (buf[len - 1U] == '\n')) {
        buf[len - 1U] = '\0';
    }

    return 0;
}

static int prompt_int(const char *prompt, int min, int max, int *value)
{
    char line[64];
    char *end;
    long parsed;

    printf("%s", prompt);
    fflush(stdout);

    if (read_line(line, sizeof(line)) != 0) {
        return -1;
    }

    parsed = strtol(line, &end, 0);
    if ((end == line) || (*end != '\0') || (parsed < min) || (parsed > max)) {
        printf("Invalid input, range is %d..%d\n", min, max);
        return -1;
    }

    *value = (int)parsed;
    return 0;
}

static void print_menu(uint8_t slave_id)
{
    printf("\n");
    printf("==== LPB3568 Modbus Master ====\n");
    printf("Current STM32 slave id: %u\n", slave_id);
    printf("1. Select STM32 slave id\n");
    printf("2. Set LED color\n");
    printf("3. Set PWM duty\n");
    printf("4. Collect LED/PWM/heartbeat\n");
    printf("5. Read one holding register\n");
    printf("6. Write one holding register\n");
    printf("q. Quit\n");
    printf("> ");
    fflush(stdout);
}

static void set_slave_id(modbus_master_t *master)
{
    int id;

    if (prompt_int("Slave id (1..247): ", 1, 247, &id) != 0) {
        return;
    }

    master->slave_id = (uint8_t)id;
    printf("Slave id set to %u\n", master->slave_id);
}

static void set_led(modbus_master_t *master)
{
    int mode;
    int ret;
    uint16_t readback;

    printf("LED color:\n");
    printf("  0 off\n");
    printf("  1 red\n");
    printf("  2 green\n");
    printf("  3 blue\n");
    printf("  4 yellow\n");
    printf("  5 purple\n");
    printf("  6 cyan\n");
    printf("  7 white\n");

    if (prompt_int("Select LED color (0..7): ", 0, 7, &mode) != 0) {
        return;
    }

    ret = modbus_write_single(master, STM32_REG_LED_MODE, (uint16_t)mode);
    if (ret != 0) {
        printf("LED write response failed: %d, checking register...\n", ret);
    }

    if (read_one(master, STM32_REG_LED_MODE, &readback) != 0) {
        printf("Set LED failed: no valid readback\n");
        return;
    }

    if (readback != (uint16_t)mode) {
        printf("Set LED failed: expected %d, readback is %u (%s)\n",
               mode, readback, led_name(readback));
        return;
    }

    printf("LED set to %s (%d)\n", led_name((uint16_t)mode), mode);
}

static void set_pwm(modbus_master_t *master)
{
    int duty;
    int ret;
    uint16_t readback;

    if (prompt_int("PWM duty (0..1000): ", 0, 1000, &duty) != 0) {
        return;
    }

    ret = modbus_write_single(master, STM32_REG_PWM_DUTY, (uint16_t)duty);
    if (ret != 0) {
        printf("PWM write response failed: %d, checking register...\n", ret);
    }

    if (read_one(master, STM32_REG_PWM_DUTY, &readback) != 0) {
        printf("Set PWM failed: no valid readback\n");
        return;
    }

    if (readback != (uint16_t)duty) {
        printf("Set PWM failed: expected %d, readback is %u\n",
               duty, readback);
        return;
    }

    printf("PWM duty set to %d/1000 (%d.%d%%)\n",
           duty, duty / 10, duty % 10);
}

static int read_one(modbus_master_t *master, uint16_t addr, uint16_t *value)
{
    return modbus_read_holding(master, addr, 1U, value);
}

static void collect_registers(modbus_master_t *master)
{
    uint16_t led;
    uint16_t pwm;
    uint16_t heartbeat;
    int ret;

    ret = read_one(master, STM32_REG_LED_MODE, &led);
    if (ret != 0) {
        printf("Read LED failed: %d\n", ret);
        return;
    }

    ret = read_one(master, STM32_REG_PWM_DUTY, &pwm);
    if (ret != 0) {
        printf("Read PWM failed: %d\n", ret);
        return;
    }

    ret = read_one(master, STM32_REG_HEARTBEAT, &heartbeat);
    if (ret != 0) {
        printf("Read heartbeat failed: %d\n", ret);
        return;
    }

    printf("Collected registers:\n");
    printf("  reg0 LED       = %u (%s)\n", led, led_name(led));
    printf("  reg1 PWM duty  = %u/1000 (%u.%u%%)\n",
           pwm, pwm / 10U, pwm % 10U);
    printf("  reg2 heartbeat = %u\n", heartbeat);
}

static void read_custom_register(modbus_master_t *master)
{
    int addr;
    uint16_t value;
    int ret;

    if (prompt_int("Register address (0..65535): ", 0, 65535, &addr) != 0) {
        return;
    }

    ret = read_one(master, (uint16_t)addr, &value);
    if (ret != 0) {
        printf("Read register %d failed: %d\n", addr, ret);
        return;
    }

    printf("Register %d = %u (0x%04X)\n", addr, value, value);
}

static void write_custom_register(modbus_master_t *master)
{
    int addr;
    int value;
    int ret;

    if (prompt_int("Register address (0..65535): ", 0, 65535, &addr) != 0) {
        return;
    }
    if (prompt_int("Value (0..65535): ", 0, 65535, &value) != 0) {
        return;
    }

    ret = modbus_write_single(master, (uint16_t)addr, (uint16_t)value);
    if (ret != 0) {
        printf("Write register %d failed: %d\n", addr, ret);
        return;
    }

    printf("Register %d written: %u (0x%04X)\n", addr, value, value);
}

int main(int argc, char **argv)
{
    const char *dev;
    int baudrate = 9600;
    int slave_id = 1;
    int fd;
    modbus_master_t master;

    if (argc < 2) {
        usage(argv[0]);
        return 2;
    }

    dev = argv[1];
    if (argc >= 3) {
        baudrate = atoi(argv[2]);
    }
    if (argc >= 4) {
        slave_id = atoi(argv[3]);
    }
    if ((slave_id <= 0) || (slave_id > 247)) {
        fprintf(stderr, "slave-id must be 1..247\n");
        return 2;
    }

    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);

    fd = serial_open(dev, baudrate);
    if (fd < 0) {
        fprintf(stderr, "open %s failed: %s\n", dev, strerror(errno));
        return 1;
    }

    modbus_master_init(&master, (uint8_t)slave_id,
                       send_serial, recv_serial, &fd);

    printf("LPB3568 Modbus RTU master: dev=%s baud=%d stm32_id=%d\n",
           dev, baudrate, slave_id);

    while (running) {
        char line[32];

        print_menu(master.slave_id);
        if (read_line(line, sizeof(line)) != 0) {
            break;
        }

        if ((strcmp(line, "q") == 0) || (strcmp(line, "Q") == 0)) {
            break;
        } else if (strcmp(line, "1") == 0) {
            set_slave_id(&master);
        } else if (strcmp(line, "2") == 0) {
            set_led(&master);
        } else if (strcmp(line, "3") == 0) {
            set_pwm(&master);
        } else if (strcmp(line, "4") == 0) {
            collect_registers(&master);
        } else if (strcmp(line, "5") == 0) {
            read_custom_register(&master);
        } else if (strcmp(line, "6") == 0) {
            write_custom_register(&master);
        } else {
            printf("Unknown command: %s\n", line);
        }
    }

    close(fd);
    printf("Bye.\n");
    return 0;
}
