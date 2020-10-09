// Copyright (c) 2015-2020, XMOS Ltd, All rights reserved
#include <xs1.h>
#include <stdio.h>
#include <stdlib.h>
#include <xcore/parallel.h>
#include <xcore/port.h>
#include <xcore/hwtimer.h>
#include <xcore/triggerable.h>
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

// static const char* ack_str(int ack)
// {
//     return (ack == I2C_ACK) ? "ack" : "nack";
// }

#define MAX_DATA_BYTES 3

typedef struct {
    volatile int done;
} i2c_op_t;

DEFINE_INTERRUPT_PERMITTED(i2c_isr_grp, void, i2c_loop,
        chanend ctrl_c,
        i2c_master_t *i2c_ctx)
{
    ;
}

I2S_CALLBACK_ATTR
static void i2c_operation_complete(i2c_master_t *ctx)
{
    i2c_op_t *op = ctx->app_data;
    op->done = 1;
}

static void i2c_master_async(void) {
    i2c_master_t i2c_ctx;

    i2c_op_t op = {
            .done = 0,
    };

    i2c_master_init(
            &i2c_ctx,
            p_scl,
            p_sda,
            100, /* kbps */
            I2CCONF_MAX_BUF_LEN,
            &op,
            i2c_operation_complete);

    INTERRUPT_PERMITTED(i2c_loop)(ctrl_c, &i2c_ctx);
}

// void test(client i2c_master_async_if i2c)
// {
//     // Have separate data arrays so that everything can be setup before starting
//     uint8_t data_write_1[MAX_DATA_BYTES] = {0};
//     uint8_t data_write_2[MAX_DATA_BYTES] = {0};
//     uint8_t data_write_3[MAX_DATA_BYTES] = {0};
//     uint8_t data_read_1[MAX_DATA_BYTES] = {0};
//     uint8_t data_read_2[MAX_DATA_BYTES] = {0};
//     int acks[NUM_TESTS] = {0};
//     size_t n1 = -1;
//     size_t n2 = -1;
//
//     size_t n3 = -1;
//
//     const int do_stop = STOP ? 1 : 0;
//
//     // Setup all data to be written
//     data_write_1[0] = 0x90; data_write_1[1] = 0xfe;
//     data_write_2[0] = 0xff; data_write_2[1] = 0x00; data_write_2[2] = 0xaa;
//     data_write_3[0] = 0xee;
//
//     // Execute all bus operations
//     i2c.write(0x3c, data_write_1, 2, do_stop);
//     select {
//         case i2c.operation_complete():
//         acks[TEST_WRITE_1] = i2c.get_write_result(n1);
//         break;
//     }
//
//     i2c.read(0x22, 2, do_stop);
//     select {
//         case i2c.operation_complete():
//         acks[TEST_READ_1] = i2c.get_read_data(data_read_1, 2);
//         break;
//     }
//
//     i2c.read(0x22, 1, do_stop);
//     select {
//         case i2c.operation_complete():
//         acks[TEST_READ_2] = i2c.get_read_data(data_read_2, 1);
//         break;
//     }
//
//     i2c.write(0x7b, data_write_2, 3, do_stop);
//     select {
//         case i2c.operation_complete():
//         acks[TEST_WRITE_2] = i2c.get_write_result(n2);
//         break;
//     }
//
//     i2c.write(0x31, data_write_3, 1, do_stop);
//     select {
//         case i2c.operation_complete():
//         acks[TEST_WRITE_3] = i2c.get_write_result(n3);
//         break;
//     }
//
//     // Print out results after all the data transactions have finished
//     printf("xCORE got %s, %d\n", ack_str(acks[TEST_WRITE_1]), n1);
//     printf("xCORE got %s, %d\n", ack_str(acks[TEST_WRITE_2]), n2);
//     printf("xCORE got %s, %d\n", ack_str(acks[TEST_WRITE_3]), n3);
//
//     printf("xCORE got %s\n", ack_str(acks[TEST_READ_1]));
//     printf("xCORE received: 0x%x, 0x%x\n", data_read_1[0], data_read_1[1]);
//     printf("xCORE got %s\n", ack_str(acks[TEST_READ_2]));
//     printf("xCORE received: 0x%x\n", data_read_2[0]);
//
//     exit(0);
// }

/** This combinable task will randomly interrupt the i2c master combinable
 *  task and add delays. This should slow down the speed but should still
 *  produce valid i2c transactions.
 */
void interference(void)
{
    hwtimer_t tmr = hwtimer_alloc();
    uint32_t timeout = hwtimer_get_time(tmr);

    triggerable_disable_all();

    TRIGGERABLE_SETUP_EVENT_VECTOR(tmr, tmr_event);
    hwtimer_set_trigger_time(tmr, timeout);

    triggerable_enable_trigger(tmr);

    while (1) {
		TRIGGERABLE_WAIT_EVENT(tmr_event);
        tmr_event: {
            int delay = rand() >> 20;
            delay_ticks(delay);
            timeout = hwtimer_get_time(tmr);
            timeout += rand() >> 20;
        	hwtimer_change_trigger_time( tmr, timeout );
			continue;
		}
    }
}

DECLARE_JOB(burn, (void));

void burn(void) {
    SETSR(XS1_SR_QUEUE_MASK | XS1_SR_FAST_MASK);
    for(;;);
}

int main(void) {
    PAR_JOBS (
// #if COMB
//   #ifdef INTERFERE
//   [[combine]]
//   par {
//     i2c_master_async_comb(i2c, 1, p_scl, p_sda, SPEED, 10);
//     interference();
//   }
//   #else
//   i2c_master_async_comb(i2c, 1, p_scl, p_sda, SPEED, 10);
//   #endif
// #else
  PJOB(i2c_master_async,());
// #endif

#ifndef INTERFERE
        PJOB(burn, ()),
#endif
        PJOB(burn, ()),
        PJOB(burn, ()),
        PJOB(burn, ()),
        PJOB(burn, ()),
        PJOB(burn, ()),
        PJOB(burn, ())
    );

    return 0;
}
