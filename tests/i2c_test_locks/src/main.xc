#include <stdio.h>
#include <xs1.h>
#include "debug_print.h"
#include "i2c.h"
#include <stdlib.h>

port p_scl = XS1_PORT_1A;
port p_sda = XS1_PORT_1B;

void task1(client i2c_master_if i2c, chanend c) {
  uint8_t data[1] = {0x99};
  size_t n;
  i2c.write(0x33, data, 1, n, 0);
  c <: 0;
  delay_ticks(1000);
  i2c.write(0x33, data, 1, n, 1);
  c <: 0;
}

void task2(client i2c_master_if i2c, chanend c) {
  uint8_t data[1] = {0x88};
  size_t n;
  c :> int;
  i2c.write(0x22, data, 1, n, 1);
  c :> int;
  exit(0);
}

int main(void) {
  i2c_master_if i2c[2];
  chan c;
  par {
    i2c_master(i2c, 2, p_scl, p_sda, 400);
    task1(i2c[0], c);
    task2(i2c[1], c);
  }
  return 0;
}