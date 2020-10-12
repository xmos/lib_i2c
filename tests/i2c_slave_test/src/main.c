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

uint8_t test_data[] = { 0xff, 0x01, 0x99, 0x20, 0x33, 0xee };

DECLARE_JOB(test, (void));

DEFINE_INTERRUPT_PERMITTED(i2c_isr_grp, void, test) {
{
    // i2c_slave(i, p_scl, p_sda, 0x3c);

    // int ack_sequence[7] = {I2C_SLAVE_ACK, I2C_SLAVE_ACK, I2C_SLAVE_NACK,
    //                        I2C_SLAVE_NACK,
    //                        I2C_SLAVE_ACK, I2C_SLAVE_NACK};
    int ack_index = 0;
    int i = 0;

    while (1) {
        // select {
        // case i2c.ack_read_request(void) -> i2c_slave_ack_t response:
        //     printstr("xCORE got start of read transaction\n");
        //     response = I2C_SLAVE_ACK;
        //     break;
        // case i2c.ack_write_request(void) -> i2c_slave_ack_t response:
        //     printstr("xCORE got start of write transaction\n");
        //     response = I2C_SLAVE_ACK;
        //     break;
        // case i2c.master_sent_data(uint8_t data) -> i2c_slave_ack_t response:
        //     printf("xCORE got data: 0x%x\n", data);
        //     if (data == 0xff) {
        //         _exit(0);
        //     }
        //     response = ack_sequence[ack_index++];
        //     break;
        // case i2c.master_requires_data() -> uint8_t data:
        //     data = test_data[i];
        //     printf("xCORE sending: 0x%x\n", data);
        //     i++;
        //     if (i >= sizeof(test_data)) {
        //         i = 0;
        //     }
        // break;
        // case i2c.stop_bit():
        //     // The stop_bit function is timing critical. Needs to use printstr to meet
        //     // timing and detect the start bit
        //     printstr("xCORE got stop bit\n");
        //     break;
        // }
    }
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
