#include "modbus_tcp_master.h"
#include "tcp_client.h"

#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#define REG_LED_MODE   0U
#define REG_PWM_DUTY   1U
#define REG_HEARTBEAT  2U

static volatile sig_atomic_t running = 1;

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

static int send_tcp(const uint8_t *data, size_t len, void *user)
{
    return tcp_write_all(*(tcp_socket_t *)user, data, len);
}

static int recv_tcp(uint8_t *data, size_t len, int timeout_ms, void *user)
{
    return tcp_read_exact_timeout(*(tcp_socket_t *)user, data, len, timeout_ms);
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

static void usage(const char *name)
{
    fprintf(stderr, "Usage: %s <lpb3568-ip> [tcp-port] [stm32-slave-id]\n",
            name);
    fprintf(stderr, "Example: %s 192.168.1.30 502 1\n", name);
}

static void print_menu(uint8_t unit_id)
{
    printf("\n");
    printf("==== MyPC Modbus TCP Master ====\n");
    printf("Current STM32 unit/slave id: %u\n", unit_id);
    printf("1. Select STM32 slave id\n");
    printf("2. Set STM32 LED color\n");
    printf("3. Set STM32 PWM duty\n");
    printf("4. Collect STM32 LED/PWM/heartbeat\n");
    printf("5. Read one holding register\n");
    printf("6. Write one holding register\n");
    printf("q. Quit\n");
    printf("> ");
    fflush(stdout);
}

static void set_unit_id(modbus_tcp_master_t *master)
{
    int id;

    if (prompt_int("STM32 slave id (1..247): ", 1, 247, &id) != 0) {
        return;
    }

    master->unit_id = (uint8_t)id;
    printf("STM32 slave id set to %u\n", master->unit_id);
}

static void set_led(modbus_tcp_master_t *master)
{
    int mode;
    int ret;
    uint16_t readback;

    printf("LED color: 0 off, 1 red, 2 green, 3 blue, 4 yellow, 5 purple, 6 cyan, 7 white\n");
    if (prompt_int("Select LED color (0..7): ", 0, 7, &mode) != 0) {
        return;
    }

    ret = modbus_tcp_write_single(master, REG_LED_MODE, (uint16_t)mode);
    if (ret != 0) {
        printf("Set LED failed: %d\n", ret);
        return;
    }

    ret = modbus_tcp_read_holding(master, REG_LED_MODE, 1U, &readback);
    if (ret != 0) {
        printf("LED write sent, but readback failed: %d\n", ret);
        return;
    }
    if (readback != (uint16_t)mode) {
        printf("LED readback mismatch: expected %d, got %u (%s)\n",
               mode, readback, led_name(readback));
        return;
    }

    printf("LED set to %s (%d)\n", led_name((uint16_t)mode), mode);
}

static void set_pwm(modbus_tcp_master_t *master)
{
    int duty;
    int ret;
    uint16_t readback;

    if (prompt_int("PWM duty (0..1000): ", 0, 1000, &duty) != 0) {
        return;
    }

    ret = modbus_tcp_write_single(master, REG_PWM_DUTY, (uint16_t)duty);
    if (ret != 0) {
        printf("Set PWM failed: %d\n", ret);
        return;
    }

    ret = modbus_tcp_read_holding(master, REG_PWM_DUTY, 1U, &readback);
    if (ret != 0) {
        printf("PWM write sent, but readback failed: %d\n", ret);
        return;
    }
    if (readback != (uint16_t)duty) {
        printf("PWM readback mismatch: expected %d, got %u\n",
               duty, readback);
        return;
    }

    printf("PWM set to %u/1000 (%u.%u%%)\n",
           readback, readback / 10U, readback % 10U);
}

static void collect_registers(modbus_tcp_master_t *master)
{
    uint16_t values[3];
    int ret = modbus_tcp_read_holding(master, REG_LED_MODE, 3U, values);

    if (ret != 0) {
        printf("Read registers failed: %d\n", ret);
        return;
    }

    printf("STM32 registers through LPB3568 gateway:\n");
    printf("  reg0 LED       = %u (%s)\n", values[0], led_name(values[0]));
    printf("  reg1 PWM duty  = %u/1000 (%u.%u%%)\n",
           values[1], values[1] / 10U, values[1] % 10U);
    printf("  reg2 heartbeat = %u\n", values[2]);
}

static void read_custom_register(modbus_tcp_master_t *master)
{
    int addr;
    uint16_t value;
    int ret;

    if (prompt_int("Register address (0..65535): ", 0, 65535, &addr) != 0) {
        return;
    }

    ret = modbus_tcp_read_holding(master, (uint16_t)addr, 1U, &value);
    if (ret != 0) {
        printf("Read register %d failed: %d\n", addr, ret);
        return;
    }

    printf("Register %d = %u (0x%04X)\n", addr, value, value);
}

static void write_custom_register(modbus_tcp_master_t *master)
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

    ret = modbus_tcp_write_single(master, (uint16_t)addr, (uint16_t)value);
    if (ret != 0) {
        printf("Write register %d failed: %d\n", addr, ret);
        return;
    }

    printf("Register %d written: %u (0x%04X)\n", addr, value, value);
}

int main(int argc, char **argv)
{
    const char *host;
    const char *port = "502";
    int unit_id = 1;
    tcp_socket_t fd;
    modbus_tcp_master_t master;

    if (argc < 2) {
        usage(argv[0]);
        return 2;
    }

    host = argv[1];
    if (argc >= 3) {
        port = argv[2];
    }
    if (argc >= 4) {
        unit_id = atoi(argv[3]);
    }
    if ((unit_id <= 0) || (unit_id > 247)) {
        fprintf(stderr, "stm32-slave-id must be 1..247\n");
        return 2;
    }

    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);

    if (tcp_client_startup() != 0) {
        fprintf(stderr, "initialize TCP failed\n");
        return 1;
    }

    fd = tcp_connect_to(host, port, 3000);
    if (!tcp_socket_valid(fd)) {
        fprintf(stderr, "connect %s:%s failed: %s\n", host, port,
                strerror(errno));
        tcp_client_cleanup();
        return 1;
    }

    modbus_tcp_master_init(&master, (uint8_t)unit_id, send_tcp, recv_tcp, &fd);
    printf("Connected to LPB3568 Modbus TCP gateway: %s:%s, STM32 id=%d\n",
           host, port, unit_id);

    while (running) {
        char line[32];

        print_menu(master.unit_id);
        if (read_line(line, sizeof(line)) != 0) {
            break;
        }

        if ((strcmp(line, "q") == 0) || (strcmp(line, "Q") == 0)) {
            break;
        } else if (strcmp(line, "1") == 0) {
            set_unit_id(&master);
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

    tcp_close(fd);
    tcp_client_cleanup();
    printf("Bye.\n");
    return 0;
}
