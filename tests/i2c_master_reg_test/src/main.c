// Copyright (c) 2014-2020, XMOS Ltd, All rights reserved
#include <xs1.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <xcore/parallel.h>
#include <xcore/port.h>
#include <xcore/hwtimer.h>
#include <xcore/triggerable.h>
#include <xcore/interrupt.h>
#include <xcore/interrupt_wrappers.h>
#include "i2c_c.h"

#define SETSR(c) asm volatile("setsr %0" : : "n"(c));

port_t p_scl = XS1_PORT_1A;
port_t p_sda = XS1_PORT_1B;

enum {
    TEST_WRITE_1 = 0,
    TEST_WRITE_2,
    TEST_WRITE_3,
    TEST_WRITE_4,
    NUM_WRITE_TESTS
};

typedef struct {
    volatile int done;
} i2c_op_t;

enum {
    TEST_READ_1 = 0,
    TEST_READ_2,
    TEST_READ_3,
    TEST_READ_4,
    NUM_READ_TESTS
};

I2C_CALLBACK_ATTR
static void i2c_operation_complete(i2c_master_t *ctx)
{
    i2c_op_t *op = ctx->app_data;
    op->done = 1;
}

#define BUF_SIZE 4

i2c_regop_res_t write_reg(i2c_master_t *ctx,
                          uint8_t device_addr,
                          uint8_t reg,
                          uint8_t data) {
    uint8_t buf[2] = {reg, data};
    ssize_t num_bytes_sent = 0;
    i2c_res_t res;
    i2c_regop_res_t reg_res;
    i2c_op_t *op = ctx->app_data;

    res = i2c_master_write(ctx, device_addr, buf, 2, 1);
    if (res == I2C_STARTED) {
        while (!op->done);
        op->done = 0;
        res = i2c_result_get(ctx, &num_bytes_sent);
        // TODO check res here as well
        if (num_bytes_sent == 0) {
            reg_res = I2C_REGOP_DEVICE_NACK;
        } else if (num_bytes_sent < 2) {
            reg_res = I2C_REGOP_INCOMPLETE;
        } else {
            reg_res = I2C_REGOP_SUCCESS;
        }
    }
    else {
        reg_res = I2C_REGOP_NOT_STARTED;
    }
    return reg_res;
}

i2c_regop_res_t write_reg8_addr16(i2c_master_t *ctx,
                                  uint8_t device_addr,
                                  uint16_t reg,
                                  uint8_t data) {
    uint8_t buf[3] = {(reg >> 8) & 0xFF, reg & 0xFF, data};
    ssize_t num_bytes_sent = 0;
    i2c_res_t res;
    i2c_regop_res_t reg_res;
    i2c_op_t *op = ctx->app_data;

    res = i2c_master_write(ctx, device_addr, buf, 3, 1);
    if (res == I2C_STARTED) {
        while (!op->done);
        op->done = 0;
        res = i2c_result_get(ctx, &num_bytes_sent);
        // TODO check res here as well
        if (num_bytes_sent == 0) {
            reg_res = I2C_REGOP_DEVICE_NACK;
        } else if (num_bytes_sent < 3) {
            reg_res = I2C_REGOP_INCOMPLETE;
        } else {
            reg_res = I2C_REGOP_SUCCESS;
        }
    }
    else {
        reg_res = I2C_REGOP_NOT_STARTED;
    }
    return reg_res;
}

i2c_regop_res_t write_reg16(i2c_master_t *ctx,
                            uint8_t device_addr,
                            uint16_t reg,
                            uint16_t data) {
    uint8_t buf[4] = {(reg >> 8) & 0xFF, reg & 0xFF, (data >> 8) & 0xFF, data & 0xFF};
    ssize_t num_bytes_sent = 0;
    i2c_res_t res;
    i2c_regop_res_t reg_res;
    i2c_op_t *op = ctx->app_data;

    res = i2c_master_write(ctx, device_addr, buf, 4, 1);
    if (res == I2C_STARTED) {
        while (!op->done);
        op->done = 0;
        res = i2c_result_get(ctx, &num_bytes_sent);
        // TODO check res here as well
        if (num_bytes_sent == 0) {
            reg_res = I2C_REGOP_DEVICE_NACK;
        } else if (num_bytes_sent < 4) {
            reg_res = I2C_REGOP_INCOMPLETE;
        } else {
            reg_res = I2C_REGOP_SUCCESS;
        }
    }
    else {
        reg_res = I2C_REGOP_NOT_STARTED;
    }
    return reg_res;
}

i2c_regop_res_t write_reg16_addr8(i2c_master_t *ctx,
                            uint8_t device_addr,
                            uint8_t reg,
                            uint16_t data) {
    uint8_t buf[3] = {reg, (data >> 8) & 0xFF, data & 0xFF};
    ssize_t num_bytes_sent = 0;
    i2c_res_t res;
    i2c_regop_res_t reg_res;
    i2c_op_t *op = ctx->app_data;

    res = i2c_master_write(ctx, device_addr, buf, 3, 1);
    if (res == I2C_STARTED) {
        while (!op->done);
        op->done = 0;
        res = i2c_result_get(ctx, &num_bytes_sent);
        // TODO check res here as well
        if (num_bytes_sent == 0) {
            reg_res = I2C_REGOP_DEVICE_NACK;
        } else if (num_bytes_sent < 3) {
            reg_res = I2C_REGOP_INCOMPLETE;
        } else {
            reg_res = I2C_REGOP_SUCCESS;
        }
    }
    else {
        reg_res = I2C_REGOP_NOT_STARTED;
    }
    return reg_res;
}

uint8_t read_reg(i2c_master_t *ctx,
                 uint8_t device_addr,
                 uint8_t reg,
                 i2c_regop_res_t *result) {
    uint8_t buf[1] = {reg};
    ssize_t num_bytes_sent = 0;
    i2c_res_t res;
    i2c_regop_res_t reg_res;
    i2c_op_t *op = ctx->app_data;

    res = i2c_master_write(ctx, device_addr, buf, 1, 0);
    if (res == I2C_STARTED) {
        while (!op->done);
        op->done = 0;
        res = i2c_result_get(ctx, &num_bytes_sent);
        // TODO check res here as well
        if (num_bytes_sent == 0) {
            reg_res = I2C_REGOP_DEVICE_NACK;
        } else if (num_bytes_sent < 1) {
            reg_res = I2C_REGOP_INCOMPLETE;
        } else {
            reg_res = I2C_REGOP_SUCCESS;
        }
    } else {
        reg_res = I2C_REGOP_NOT_STARTED;
    }
    memset(buf, 0x00, 1);
    if(reg_res != I2C_REGOP_SUCCESS) {
        i2c_master_stop_bit_send(ctx);
    } else {
        res = i2c_master_read(ctx, device_addr, buf, 1, 1);
        if (res == I2C_STARTED) {
            while (!op->done);
            op->done = 0;
            res = i2c_result_get(ctx, &num_bytes_sent);
            if (num_bytes_sent == 0) {
                reg_res = I2C_REGOP_DEVICE_NACK;
            } else if (num_bytes_sent < 1) {
                reg_res = I2C_REGOP_INCOMPLETE;
            } else {
                reg_res = I2C_REGOP_SUCCESS;
            }
        } else {
            reg_res = I2C_REGOP_NOT_STARTED;
        }
    }

    *result = reg_res;
    return buf[0];
}

uint8_t read_reg8_addr16(i2c_master_t *ctx,
                         uint8_t device_addr,
                         uint16_t reg,
                         i2c_regop_res_t *result) {
    uint8_t buf[2] = {(reg >> 8) & 0xFF, reg & 0xFF};
    ssize_t num_bytes_sent = 0;
    i2c_res_t res;
    i2c_regop_res_t reg_res;
    i2c_op_t *op = ctx->app_data;

    res = i2c_master_write(ctx, device_addr, buf, 2, 0);
    if (res == I2C_STARTED) {
        while (!op->done);
        op->done = 0;
        res = i2c_result_get(ctx, &num_bytes_sent);
        // TODO check res here as well
        if (num_bytes_sent == 0) {
            reg_res = I2C_REGOP_DEVICE_NACK;
        } else if (num_bytes_sent < 2) {
            reg_res = I2C_REGOP_INCOMPLETE;
        } else {
            reg_res = I2C_REGOP_SUCCESS;
        }
    } else {
        reg_res = I2C_REGOP_NOT_STARTED;
    }
    memset(buf, 0x00, 2);
    if(reg_res != I2C_REGOP_SUCCESS) {
        i2c_master_stop_bit_send(ctx);
    } else {
        res = i2c_master_read(ctx, device_addr, buf, 1, 1);
        if (res == I2C_STARTED) {
            while (!op->done);
            op->done = 0;
            res = i2c_result_get(ctx, &num_bytes_sent);
            if (num_bytes_sent == 0) {
                reg_res = I2C_REGOP_DEVICE_NACK;
            } else if (num_bytes_sent < 1) {
                reg_res = I2C_REGOP_INCOMPLETE;
            } else {
                reg_res = I2C_REGOP_SUCCESS;
            }
        } else {
            reg_res = I2C_REGOP_NOT_STARTED;
        }
    }

    *result = reg_res;
    return buf[0];
}

uint16_t read_reg16(i2c_master_t *ctx,
                    uint8_t device_addr,
                    uint16_t reg,
                    i2c_regop_res_t *result) {
    uint8_t buf[2] = {(reg >> 8) & 0xFF, reg & 0xFF};
    ssize_t num_bytes_sent = 0;
    i2c_res_t res;
    i2c_regop_res_t reg_res;
    i2c_op_t *op = ctx->app_data;

    res = i2c_master_write(ctx, device_addr, buf, 2, 0);
    if (res == I2C_STARTED) {
        while (!op->done);
        op->done = 0;
        res = i2c_result_get(ctx, &num_bytes_sent);
        // TODO check res here as well
        if (num_bytes_sent == 0) {
            reg_res = I2C_REGOP_DEVICE_NACK;
        } else if (num_bytes_sent < 2) {
            reg_res = I2C_REGOP_INCOMPLETE;
        } else {
            reg_res = I2C_REGOP_SUCCESS;
        }
    } else {
        reg_res = I2C_REGOP_NOT_STARTED;
    }
    memset(buf, 0x00, 2);
    if(reg_res != I2C_REGOP_SUCCESS) {
        i2c_master_stop_bit_send(ctx);
    } else {
        res = i2c_master_read(ctx, device_addr, buf, 2, 1);
        if (res == I2C_STARTED) {
            while (!op->done);
            op->done = 0;
            res = i2c_result_get(ctx, &num_bytes_sent);
            if (num_bytes_sent == 0) {
                reg_res = I2C_REGOP_DEVICE_NACK;
            } else if (num_bytes_sent < 2) {
                reg_res = I2C_REGOP_INCOMPLETE;
            } else {
                reg_res = I2C_REGOP_SUCCESS;
            }
        } else {
            reg_res = I2C_REGOP_NOT_STARTED;
        }
    }

    *result = reg_res;
    return (uint16_t)(buf[0] << 8 | buf[1]);
}

uint16_t read_reg16_addr8(i2c_master_t *ctx,
                          uint8_t device_addr,
                          uint8_t reg,
                          i2c_regop_res_t *result) {
    uint8_t buf[2] = {reg, 0x00};
    ssize_t num_bytes_sent = 0;
    i2c_res_t res;
    i2c_regop_res_t reg_res;
    i2c_op_t *op = ctx->app_data;

    res = i2c_master_write(ctx, device_addr, buf, 1, 0);
    if (res == I2C_STARTED) {
        while (!op->done);
        op->done = 0;
        res = i2c_result_get(ctx, &num_bytes_sent);
        // TODO check res here as well
        if (num_bytes_sent == 0) {
            reg_res = I2C_REGOP_DEVICE_NACK;
        } else if (num_bytes_sent < 1) {
            reg_res = I2C_REGOP_INCOMPLETE;
        } else {
            reg_res = I2C_REGOP_SUCCESS;
        }
    } else {
        reg_res = I2C_REGOP_NOT_STARTED;
    }
    memset(buf, 0x00, 2);
    if(reg_res != I2C_REGOP_SUCCESS) {
        i2c_master_stop_bit_send(ctx);
    } else {
        res = i2c_master_read(ctx, device_addr, buf, 2, 1);
        if (res == I2C_STARTED) {
            while (!op->done);
            op->done = 0;
            res = i2c_result_get(ctx, &num_bytes_sent);
            if (num_bytes_sent == 0) {
                reg_res = I2C_REGOP_DEVICE_NACK;
            } else if (num_bytes_sent < 2) {
                reg_res = I2C_REGOP_INCOMPLETE;
            } else {
                reg_res = I2C_REGOP_SUCCESS;
            }
        } else {
            reg_res = I2C_REGOP_NOT_STARTED;
        }
    }

    *result = reg_res;
    return (uint16_t)(buf[0] << 8 | buf[1]);
}

DECLARE_JOB(test, (void));

DEFINE_INTERRUPT_PERMITTED(i2c_isr_grp, void, test) {
    i2c_master_t i2c_ctx;
    i2c_master_t* i2c_ctx_ptr = &i2c_ctx;
    i2c_op_t op = {
            .done = 0,
    };

    i2c_regop_res_t write_results[NUM_WRITE_TESTS];
    i2c_regop_res_t read_results[NUM_READ_TESTS];

    i2c_master_init(
            i2c_ctx_ptr,
            p_scl,
            p_sda,
            400, /* kbps */
            (NUM_WRITE_TESTS > NUM_READ_TESTS) ? NUM_WRITE_TESTS : NUM_READ_TESTS,
            &op,
            i2c_operation_complete);

    SETSR(XS1_SR_QUEUE_MASK | XS1_SR_FAST_MASK);

    interrupt_unmask_all();

    // Test register writing
    write_results[TEST_WRITE_1] = write_reg(i2c_ctx_ptr, 0x44, 0x07, 0x12);
    write_results[TEST_WRITE_2] = write_reg8_addr16(i2c_ctx_ptr, 0x22, 0xfe99, 0x12);
    write_results[TEST_WRITE_3] = write_reg16(i2c_ctx_ptr, 0x33, 0xabcd, 0x12a3);
    write_results[TEST_WRITE_4] = write_reg16_addr8(i2c_ctx_ptr, 0x11, 0xef, 0x4567);

    // Test register reading
    unsigned vals[NUM_READ_TESTS] = {0};
    vals[TEST_READ_1] = read_reg(i2c_ctx_ptr, 0x44, 0x33, &read_results[TEST_READ_1]);
    vals[TEST_READ_2] = read_reg8_addr16(i2c_ctx_ptr, 0x45, 0xa321, &read_results[TEST_READ_2]);
    vals[TEST_READ_3] = read_reg16(i2c_ctx_ptr, 0x46, 0x3399, &read_results[TEST_READ_3]);
    vals[TEST_READ_4] = read_reg16_addr8(i2c_ctx_ptr, 0x47, 0x22, &read_results[TEST_READ_4]);

    // Print all the results
    for (size_t i = 0; i < NUM_WRITE_TESTS; ++i) {
        printf(write_results[i] == I2C_REGOP_SUCCESS ? "ACK\n" : "NACK\n");
    }

    for (size_t i = 0; i < NUM_READ_TESTS; ++i) {
        printf(read_results[i] == I2C_REGOP_SUCCESS ? "ACK\n" : "NACK\n");
        printf("val=%X\n", vals[i]);
    }

    exit(0);
}

DECLARE_JOB(burn, (void));

void burn(void) {
    SETSR(XS1_SR_QUEUE_MASK | XS1_SR_FAST_MASK);
    for(;;);
}

int main(void) {
    PAR_JOBS (
        PJOB(INTERRUPT_PERMITTED(test),()),
        PJOB(burn, ()),
        PJOB(burn, ()),
        PJOB(burn, ()),
        PJOB(burn, ()),
        PJOB(burn, ()),
        PJOB(burn, ()),
        PJOB(burn, ())
    );

    return 0;
}
