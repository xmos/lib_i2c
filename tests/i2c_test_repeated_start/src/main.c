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

typedef struct {
    volatile int done;
} i2c_op_t;

I2C_CALLBACK_ATTR
static void i2c_operation_complete(i2c_master_t *ctx)
{
    i2c_op_t *op = ctx->app_data;
    op->done = 1;
}

DECLARE_JOB(test, (void));

DEFINE_INTERRUPT_PERMITTED(i2c_isr_grp, void, test) {
    i2c_master_t i2c_ctx;
    i2c_master_t* i2c_ctx_ptr = &i2c_ctx;
    i2c_op_t op = {
            .done = 0,
    };

    uint8_t data[1] = {0x99};
    size_t numbytes;

    i2c_master_init(
            i2c_ctx_ptr,
            p_scl,
            p_sda,
            400, /* kbps */
            1,
            &op,
            i2c_operation_complete);

    SETSR(XS1_SR_QUEUE_MASK | XS1_SR_FAST_MASK);

    interrupt_unmask_all();

    i2c_master_write(i2c_ctx_ptr, 0x33, data, 1, 0);
    i2c_master_write(i2c_ctx_ptr, 0x33, data, 1, 1);

    exit(0);
}

DECLARE_JOB(burn, (void));

void burn(void) {
    SETSR(XS1_SR_QUEUE_MASK | XS1_SR_FAST_MASK);
    for(;;);
}

int main(void) {
    INTERRUPT_PERMITTED(test)();
    return 0;
}
