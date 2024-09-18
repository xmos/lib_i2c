// Copyright 2014-2024 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <stdio.h>
#include <xs1.h>
#include "debug_print.h"
#include "i2c.h"
#include <stdlib.h>

port p_scl = XS1_PORT_1A;
port p_sda = XS1_PORT_1B;

enum {
  TEST_WRITE_1 = 0,
  TEST_WRITE_2,
  TEST_WRITE_3,
  TEST_WRITE_4,
  NUM_WRITE_TESTS
};

enum {
  TEST_READ_1 = 0,
  TEST_READ_2,
  TEST_READ_3,
  TEST_READ_4,
  NUM_READ_TESTS
};

void test(client i2c_master_if i2c)
{
  i2c_regop_res_t write_results[NUM_WRITE_TESTS];
  i2c_regop_res_t read_results[NUM_READ_TESTS];

  write_results[TEST_WRITE_1] = i2c.write_reg(0x44, 0x07, 0x12);
  write_results[TEST_WRITE_2] = i2c.write_reg8_addr16(0x22, 0xfe99, 0x12);
  write_results[TEST_WRITE_3] = i2c.write_reg16(0x33, 0xabcd, 0x12a3);
  write_results[TEST_WRITE_4] = i2c.write_reg16_addr8(0x11, 0xef, 0x4567);

  // Test register reading
  unsigned vals[NUM_READ_TESTS];
  vals[TEST_READ_1] = i2c.read_reg(0x44, 0x33, read_results[TEST_READ_1]);
  vals[TEST_READ_2] = i2c.read_reg8_addr16(0x45, 0xa321, read_results[TEST_READ_2]);
  vals[TEST_READ_3] = i2c.read_reg16(0x46, 0x3399, read_results[TEST_READ_3]);
  vals[TEST_READ_4] = i2c.read_reg16_addr8(0x47, 0x22, read_results[TEST_READ_4]);

  // Print all the results
  for (size_t i = 0; i < NUM_WRITE_TESTS; ++i) {
    debug_printf(write_results[i] == I2C_REGOP_SUCCESS ? "XCORE: ACK\n" : "XCORE: NACK\n");
  }

  for (size_t i = 0; i < NUM_READ_TESTS; ++i) {
    debug_printf(read_results[i] == I2C_REGOP_SUCCESS ? "XCORE: ACK\n" : "XCORE: NACK\n");
    debug_printf("XCORE: val=%x\n", vals[i]);
  }
  exit(0);
}

int main(void) {
  i2c_master_if i2c[1];
  par {
    i2c_master(i2c, 1, p_scl, p_sda, 400);
    {set_core_fast_mode_on();test(i2c[0]);}
    par(int i=0;i<7;i++) while(1);
  }
  return 0;
}
