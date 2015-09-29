// Copyright (c) 2015, XMOS Ltd, All rights reserved
#include <stdio.h>
#include <xs1.h>
#include "debug_print.h"
#include "i2c.h"
#include <stdlib.h>

port p_scl = XS1_PORT_1A;
port p_sda = XS1_PORT_1B;

void task1(client i2c_master_if i2c) {
  uint8_t data[1] = {0x99};
  size_t numbytes;
  i2c.write(0x33, data, 1, numbytes, 0);
  i2c.write(0x33, data, 1, numbytes, 1);
  exit(0);
}


int main(void) {
  i2c_master_if i2c[1];
  par {
    i2c_master(i2c, 1, p_scl, p_sda, 400);
    task1(i2c[0]);
  }
  return 0;
}
