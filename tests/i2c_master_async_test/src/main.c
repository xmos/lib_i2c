// Copyright (c) 2015-2020, XMOS Ltd, All rights reserved
#include <xs1.h>
#include <stdio.h>
#include <stdlib.h>
#include <xcore/parallel.h>
#include <xcore/port.h>
#include <xcore/hwtimer.h>
#include <xcore/triggerable.h>
#include <xcore/interrupt_wrappers.h>
#include <xcore/interrupt.h>
#include "i2c_c.h"

#define SETSR(c) asm volatile("setsr %0" : : "n"(c));

port_t p_scl = XS1_PORT_1A;
port_t p_sda = XS1_PORT_1B;

// Define the same tests as the i2c_master_test so that the expect files are the
// same
enum {
    TEST_WRITE_1 = 0,
    TEST_READ_1,
    TEST_READ_2,
    TEST_WRITE_2,
    TEST_WRITE_3,
    NUM_TESTS
};

static const char* ack_str(i2c_res_t ack) {
    return (ack == I2C_ACK) ? "ack" : "nack";
}

#define MAX_DATA_BYTES 3

typedef struct {
    volatile int done;
} i2c_op_t;


I2C_CALLBACK_ATTR
static void i2c_operation_complete(i2c_master_async_t *ctx)
{
    i2c_op_t *op = ctx->app_data;
    op->done = 1;
}

static void interference() {
#ifdef INTERFERE
    static uint32_t timeout;
    uint32_t now = get_reference_time();

    interrupt_mask_all();
    {
        if (now >= timeout) {
            int delay = rand() >> 20;
            while ((get_reference_time() - now) < delay);
            timeout = get_reference_time();
            timeout += rand() >> 20;
        }
    }
    interrupt_unmask_all();
#endif
}

DEFINE_INTERRUPT_PERMITTED(i2c_isr_grp, void, i2c_master_async_test,
        i2c_master_async_t *i2c_ctx) {
    // Have separate data arrays so that everything can be setup before starting
    uint8_t data_write_1[MAX_DATA_BYTES] = {0};
    uint8_t data_write_2[MAX_DATA_BYTES] = {0};
    uint8_t data_write_3[MAX_DATA_BYTES] = {0};
    uint8_t data_read_1[MAX_DATA_BYTES] = {0};
    uint8_t data_read_2[MAX_DATA_BYTES] = {0};
    int acks[NUM_TESTS] = {0};
    size_t n1 = -1;
    size_t n2 = -1;
    size_t n3 = -1;
    i2c_res_t res;
    i2c_op_t *op = &i2c_ctx->app_data;

    const int do_stop = STOP ? 1 : 0;

    interrupt_unmask_all();

    // Setup all data to be written
    data_write_1[0] = 0x90; data_write_1[1] = 0xfe;
    data_write_2[0] = 0xff; data_write_2[1] = 0x00; data_write_2[2] = 0xaa;
    data_write_3[0] = 0xee;

    // Execute all bus operations
    do {
        res = i2c_master_async_write(i2c_ctx, 0x3c, data_write_1, 2, do_stop);
    } while( res != I2C_STARTED);
    while (!op->done) {
        interference();
    }
    op->done = 0;
    acks[TEST_WRITE_1] = i2c_master_async_result_get(i2c_ctx, &n1);

    do {
        res = i2c_master_async_read(i2c_ctx, 0x22, data_read_1, 2, do_stop);
    } while( res != I2C_STARTED);
    while (!op->done) {
        interference();
    }
    op->done = 0;
    acks[TEST_READ_1] = i2c_master_async_result_get(i2c_ctx, NULL);

    do {
        res = i2c_master_async_read(i2c_ctx, 0x22, data_read_2, 1, do_stop);
    } while( res != I2C_STARTED);
    while (!op->done) {
        interference();
    }
    op->done = 0;
    acks[TEST_READ_2] = i2c_master_async_result_get(i2c_ctx, NULL);

    do {
        res = i2c_master_async_write(i2c_ctx, 0x7b, data_write_2, 3, do_stop);
    } while( res != I2C_STARTED);
    while (!op->done) {
        interference();
    }
    op->done = 0;
    acks[TEST_WRITE_2] = i2c_master_async_result_get(i2c_ctx, &n2);

    do {
        res = i2c_master_async_write(i2c_ctx, 0x31, data_write_3, 1, do_stop);
    } while( res != I2C_STARTED);
    while (!op->done) {
        interference();
    }
    op->done = 0;
    acks[TEST_WRITE_3] = i2c_master_async_result_get(i2c_ctx, &n3);

    // Print out results after all the data transactions have finished
    printf("xCORE got %s, %d\n", ack_str(acks[TEST_WRITE_1]), n1);
    printf("xCORE got %s, %d\n", ack_str(acks[TEST_WRITE_2]), n2);
    printf("xCORE got %s, %d\n", ack_str(acks[TEST_WRITE_3]), n3);

    printf("xCORE got %s\n", ack_str(acks[TEST_READ_1]));
    printf("xCORE received: 0x%X, 0x%X\n", data_read_1[0], data_read_1[1]);
    printf("xCORE got %s\n", ack_str(acks[TEST_READ_2]));
    printf("xCORE received: 0x%X\n", data_read_2[0]);

    exit(0);
}

DECLARE_JOB(burn, (void));

void burn(void) {
    SETSR(XS1_SR_QUEUE_MASK | XS1_SR_FAST_MASK);
    for(;;);
}

int main(void) {
    i2c_master_async_t i2c_ctx;

    i2c_op_t op = {
            .done = 0,
    };

    i2c_master_async_init(
            &i2c_ctx,
            p_scl,
            p_sda,
            SPEED, /* kbps */
            &op,
            i2c_operation_complete);

    PAR_JOBS (
        PJOB(INTERRUPT_PERMITTED(i2c_master_async_test),(&i2c_ctx)),
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
