// Copyright (c) 2018-2019, XMOS Ltd, All rights reserved

/* A simple application example used for code snippets in the library
 * documentation.
 */

#include <xs1.h>
#include <stdio.h>
#include "i2c.h"

void my_application(server i2c_slave_callback_if i2c);

// I2C interface ports
port p_scl = XS1_PORT_1E;
port p_sda = XS1_PORT_1F;

int main(void) {
  static const uint8_t device_addr = 0x3c;
  i2c_slave_callback_if i2c;

  par {
    i2c_slave(i2c, p_scl, p_sda, device_addr);
    my_application(i2c);
  }

  return 0;
}

void my_application(server i2c_slave_callback_if i2c) {
  while (1) {
    select {
    case i2c.ack_read_request() -> i2c_slave_ack_t response:
      response = I2C_SLAVE_ACK;
      break;
    case i2c.ack_write_request() -> i2c_slave_ack_t response:
      response = I2C_SLAVE_ACK;
      break;
    case i2c.master_sent_data(uint8_t data) -> i2c_slave_ack_t response:
      // handle write to device here, set response to NACK for the
      // last byte of data in the transaction.
      break;
    case i2c.master_requires_data() -> uint8_t data:
      // handle read from device here
      break;
    case i2c.stop_bit():
      break;
    }
  }
}

// end
