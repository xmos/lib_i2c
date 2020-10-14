// Copyright (c) 2014-2020, XMOS Ltd, All rights reserved
#include <xs1.h>
#include <stdio.h>
#include <stdlib.h>
#include <xcore/parallel.h>
#include <xcore/port.h>
#include <xcore/hwtimer.h>
#include <xcore/triggerable.h>
#include <xcore/interrupt.h>
#include <xcore/interrupt_wrappers.h>
#include "i2c_c.h"

#define SETSR(c) asm volatile("setsr %0" : : "n"(c));

port_t p_i2c = XS1_PORT_8A;

// Test the following pairs of operations:
// write -> write
// write -> read
// read  -> read
// read  -> write
enum {
    TEST_WRITE_1 = 0,
    TEST_WRITE_2,
    TEST_READ_1,
    TEST_READ_2,
    TEST_WRITE_3,
    NUM_TESTS
};

static const char* ack_str(i2c_res_t ack)
{
    return (ack == I2C_ACK) ? "ack" : "nack";
}

#define MAX_DATA_BYTES 3

DECLARE_JOB(test, (void));


void test()
{
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

    const int do_stop = STOP ? 1 : 0;

    i2c_master_sp_ctx_t i2c_ctx;
    i2c_master_sp_ctx_t* i2c_ctx_ptr = &i2c_ctx;
    hwtimer_t tmr;

    tmr = hwtimer_alloc();

    i2c_master_sp_init(
            i2c_ctx_ptr,
            p_i2c,
            tmr,
            SPEED, /* kbps */
            1,
            3,
            0);

    SETSR(XS1_SR_QUEUE_MASK | XS1_SR_FAST_MASK);

    // Setup all data to be written
    data_write_1[0] = 0x90; data_write_1[1] = 0xfe;
    data_write_2[0] = 0xff; data_write_2[1] = 0x00; data_write_2[2] = 0xaa;
    data_write_3[0] = 0xee;

    // Execute all bus operations
    acks[TEST_WRITE_1] = i2c_master_sp_write(i2c_ctx_ptr, 0x3c, data_write_1, 2, &n1, do_stop);
    acks[TEST_WRITE_2] = i2c_master_sp_write(i2c_ctx_ptr, 0x7b, data_write_2, 3, &n2, do_stop);
    acks[TEST_READ_1] = i2c_master_sp_read(i2c_ctx_ptr, 0x22, data_read_1, 2, do_stop);
    acks[TEST_READ_2] = i2c_master_sp_read(i2c_ctx_ptr, 0x88, data_read_2, 3, do_stop);
    acks[TEST_WRITE_3] = i2c_master_sp_write(i2c_ctx_ptr, 0x31, data_write_3, 1, &n3, do_stop);

    // Print out results after all the data transactions have finished
    printf("xCORE got %s, %d\n", ack_str(acks[TEST_WRITE_1]), n1);
    printf("xCORE got %s, %d\n", ack_str(acks[TEST_WRITE_2]), n2);
    printf("xCORE got %s\n", ack_str(acks[TEST_READ_1]));
    printf("xCORE received: 0x%X, 0x%X\n", data_read_1[0], data_read_1[1]);
    printf("xCORE got %s\n", ack_str(acks[TEST_READ_2]));
    printf("xCORE received: 0x%X, 0x%X, 0x%X\n", data_read_2[0], data_read_2[1], data_read_2[2]);
    printf("xCORE got %s, %d\n", ack_str(acks[TEST_WRITE_3]), n3);

    // Done - stop test
    exit(0);
}

DECLARE_JOB(burn, (void));

void burn(void) {
    SETSR(XS1_SR_QUEUE_MASK | XS1_SR_FAST_MASK);
    for(;;);
}

int main(void) {
    PAR_JOBS (
        PJOB(test, ()),
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
