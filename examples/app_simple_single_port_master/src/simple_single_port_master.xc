// Copyright 2018-2025 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

/* A simple application example used for code snippets in the library
 * documentation.
 */

#include <xs1.h>
#include <stdio.h>
#include "i2c.h"

void my_application(client i2c_master_if i2c, uint8_t target_device_addr);

// I2C interface ports
port p_i2c = XS1_PORT_4C;

int main(void) {
  i2c_master_if i2c[1];
  static const uint8_t target_device_addr = 0x3c;

  par {
    i2c_master_single_port(i2c, 1, p_i2c, 100, 1, 3, 0);
    my_application(i2c[0], target_device_addr);
  }
  return 0;
}

void my_application(client i2c_master_if i2c, uint8_t target_device_addr) {
  uint8_t data[2];
  i2c.read(target_device_addr, data, 2, 1);
  printf("Read data %d, %d from the bus.\n", data[0], data[1]);
}

// end
