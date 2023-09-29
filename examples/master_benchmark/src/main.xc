#include <platform.h>
#include <xs1.h>

#include "debug_print.h"

#include "i2c_master_if.h"

port p_scl = PORT_I2C_SCL;
port p_sda = PORT_I2C_SDA;
out port p_ctrl = PORT_CTRL;

void ctrlPort() {
    for (int i = 0; i < 30; ++i) {
        p_ctrl <: 0x30;
        delay_microseconds(5);
        p_ctrl <: 0x20;
        delay_microseconds(5);
    }

    delay_milliseconds(100);
}

#define ITERS 10000

static void read_i2c(client interface i2c_master_if i2c, uint8_t addr, uint8_t reg) {
    i2c_res_t result;
    uint8_t a_reg[1] = {reg};
    uint8_t data[1] = {0};
    size_t n;

    result = i2c.write(addr, a_reg, 1, n, 0);

    if (n != 1) {
        i2c.send_stop_bit();
        asm volatile("ecallf %0"::"r"(0));
    }

    result = i2c.read(addr, data, 1, 1);
    if (result != I2C_ACK) {
        asm volatile("ecallf %0"::"r"(0));
    }
}

static void write_i2c(client interface i2c_master_if i2c, uint8_t addr, uint8_t reg, uint8_t data) {
    uint8_t a_data[2] = {reg, data};
    size_t n;

    i2c.write(addr, a_data, 2, n, 1);

    if (n == 0) {
        asm volatile("ecallf %0"::"r"(0));
    } else if (n < 2) {
        asm volatile("ecallf %0"::"r"(0));
    }
}

void benchmark_task(client interface i2c_master_if i2c) {
    delay_seconds(1);

    timer tmr;
    unsigned t1;
    tmr :> t1;

    write_i2c(i2c, 0x70, 0, 0x04);

    for (int i = 0; i < ITERS; ++i) {
        read_i2c(i2c, 0x4C, 0x00);
        write_i2c(i2c, 0x4C, 0x00, 0x01);
        read_i2c(i2c, 0x4C, 0x00);
        write_i2c(i2c, 0x4C, 0x00, 0x00);
    }

    i2c.shutdown();

    unsigned t2;
    tmr :> t2;

    debug_printf("%d\n", t2 - t1);
}

int main() {
    interface i2c_master_if i2c[1];

    par {
        {
            ctrlPort();
            i2c_master(i2c, 1, p_scl, p_sda, 100);
        };
        benchmark_task(i2c[0]);
    }

    return 0;
}
