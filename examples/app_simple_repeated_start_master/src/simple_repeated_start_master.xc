// Copyright (c) 2018-2019, XMOS Ltd, All rights reserved

/* A simple application example used for code snippets in the library
 * documentation.
 */

#include <xs1.h>
#include <stdio.h>
#include "i2c.h"

void my_application(client i2c_master_if i2c, uint8_t target_device_addr);

// I2C interface ports
port p_scl = XS1_PORT_1E;
port p_sda = XS1_PORT_1F;

static const uint8_t target_device_addr = 0x3c;

int main(void) {
  i2c_master_if i2c[1];
  par {
    i2c_master(i2c, 1, p_scl, p_sda, 100);
    my_application(i2c[0], target_device_addr);
  }
  return 0;
}

void my_application(client i2c_master_if i2c, uint8_t target_device_addr) {
  uint8_t data[2] = { 0x1, 0x2 };
  size_t num_bytes_sent = 0;

  // Do a write operation with no stop bit
  i2c.write(target_device_addr, data, 2, num_bytes_sent, 0);

  // This operation will begin with a repeated start bit
  i2c.read(target_device_addr, data, 2, 1);
  printf("Read data %d, %d from the bus.\n", data[0], data[1]);
}

// end
