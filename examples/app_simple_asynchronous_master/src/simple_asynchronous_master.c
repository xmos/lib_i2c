// Copyright (c) 2018-2020, XMOS Ltd, All rights reserved

/* A simple application example used for code snippets in the library
 * documentation.
 */

 #include <xs1.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <xcore/parallel.h>
 #include <xcore/port.h>
 #include <xcore/hwtimer.h>
 #include <xcore/triggerable.h>
 #include <xcore/interrupt_wrappers.h>
 #include <xcore/interrupt.h>
 #include "i2c.h"

DECLARE_INTERRUPT_PERMITTED(void, my_application, i2c_master_async_t *i2c_ctx, uint8_t target_device_addr);
// void my_application(client i2c_master_async_if i2c, uint8_t target_device_addr);
DECLARE_JOB(dummy_thread, (void));
void my_application_handle_bus_error(i2c_res_t result);
void my_application_fill_buffer(uint8_t buffer[]);

// I2C interface ports
port_t p_scl = XS1_PORT_1N;
port_t p_sda = XS1_PORT_1O;

#define BUFFER_BYTES 100

typedef struct {
    volatile int done;
} i2c_op_t;


I2C_CALLBACK_ATTR
static void i2c_operation_complete(i2c_master_async_t *ctx)
{
    i2c_op_t *op = ctx->app_data;
    op->done = 1;
}

int main(void) {
    i2c_master_async_t i2c_ctx;
    static const uint8_t target_device_addr = 0x3c;

    i2c_op_t op = {
            .done = 0,
    };

    i2c_master_async_init(
            &i2c_ctx,
            p_scl,
            p_sda,
            100, /* kbps */
            &op,
            i2c_operation_complete);

    PAR_JOBS (
        PJOB(INTERRUPT_PERMITTED(my_application),(&i2c_ctx, target_device_addr)),
        PJOB(dummy_thread, ())
    );

    return 0;
}

void dummy_thread(void) {
    for(;;);
}

DEFINE_INTERRUPT_PERMITTED(i2c_isr_grp, void, my_application,
        i2c_master_async_t *i2c_ctx, uint8_t target_device_addr) {
    uint8_t buffer[2][BUFFER_BYTES];
    size_t num_bytes_sent;
    i2c_res_t result;
    static int buf_num = 0;
    i2c_op_t *op = i2c_ctx->app_data;

    interrupt_unmask_all();

    // Create and send initial block of data
    my_application_fill_buffer(buffer[buf_num]);
    do {
        result = i2c_master_async_write(i2c_ctx, target_device_addr, buffer[buf_num], BUFFER_BYTES, 1);
    } while( result != I2C_STARTED);

    // Compute the next block of data
    buf_num ^= 1;
    my_application_fill_buffer(buffer[buf_num]);
    while (1) {
        // Wait until the last operation is complete
        while (!op->done) {;}
        op->done = 0;

        result = i2c_master_async_result_get(i2c_ctx, &num_bytes_sent);
        if (num_bytes_sent != BUFFER_BYTES) {
           my_application_handle_bus_error(result);
        }

        // Request the next data bytes to be sent
        do {
            result = i2c_master_async_write(i2c_ctx, target_device_addr, buffer[buf_num], BUFFER_BYTES, 1);
        } while( result != I2C_STARTED);

        buf_num ^= 1;

        // Computing and send the next block of data
        my_application_fill_buffer(buffer[buf_num]);
    }
}

void my_application_handle_bus_error(i2c_res_t result) {
    // Ignore for now
}

void my_application_fill_buffer(uint8_t buffer[]) {
    static int offset = 0;
    for (int i = 0; i < BUFFER_BYTES; ++i) {
        buffer[i] = i + offset;
    }
    offset += 1;
}

// end
