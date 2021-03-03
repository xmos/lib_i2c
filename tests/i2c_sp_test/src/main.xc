// Copyright (c) 2014-2021, XMOS Ltd, All rights reserved
// This software is available under the terms provided in LICENSE.txt.
#include <stdio.h>
#include <xs1.h>
#include "debug_print.h"
#include "i2c.h"
#include <stdlib.h>

port p_i2c = XS1_PORT_8A;

// Test the following pairs of operations:
//  write,write
//  write,read
//  read,read
//  read,write
enum {
  TEST_WRITE_1 = 0,
  TEST_WRITE_2,
  TEST_READ_1,
  TEST_READ_2,
  TEST_WRITE_3,
  NUM_TESTS
};

static const char * unsafe ack_str(int ack)
{
  unsafe {
    return (ack == I2C_ACK) ? "ack" : "nack";
  }
}

#define MAX_DATA_BYTES 3

void test(client i2c_master_if i2c)
{
  // Have separate data arrays so that everything can be setup before starting
  uint8_t data_write_1[MAX_DATA_BYTES] = {0};
  uint8_t data_write_2[MAX_DATA_BYTES] = {0};
  uint8_t data_write_3[MAX_DATA_BYTES] = {0};
  uint8_t data_read_1[MAX_DATA_BYTES] = {0};
  uint8_t data_read_2[MAX_DATA_BYTES] = {0};
  int acks[NUM_TESTS] = {0};
  size_t n1 = -1;
  size_t n2 = -1;
  size_t n3 = -1;

  const int do_stop = STOP ? 1 : 0;

  // Setup all data to be written
  data_write_1[0] = 0x90; data_write_1[1] = 0xfe;
  data_write_2[0] = 0xff; data_write_2[1] = 0x00; data_write_2[2] = 0xaa;
  data_write_3[0] = 0xee;

  // Execute all bus operations
  acks[TEST_WRITE_1] = i2c.write(0x3c, data_write_1, 2, n1, do_stop);
  acks[TEST_WRITE_2] = i2c.write(0x7b, data_write_2, 3, n2, do_stop);
  acks[TEST_READ_1] = i2c.read(0x22, data_read_1, 2, do_stop);
  acks[TEST_READ_2] = i2c.read(0x88, data_read_2, 3, do_stop);
  acks[TEST_WRITE_3] = i2c.write(0x31, data_write_3, 1, n3, do_stop);

  // Print out results after all the data transactions have finished
  unsafe {
    debug_printf("xCORE got %s, %d\n", ack_str(acks[TEST_WRITE_1]), n1);
    debug_printf("xCORE got %s, %d\n", ack_str(acks[TEST_WRITE_2]), n2);
    debug_printf("xCORE got %s\n", ack_str(acks[TEST_READ_1]));
    debug_printf("xCORE received: 0x%x, 0x%x\n", data_read_1[0], data_read_1[1]);
    debug_printf("xCORE got %s\n", ack_str(acks[TEST_READ_2]));
    debug_printf("xCORE received: 0x%x, 0x%x, 0x%x\n", data_read_2[0], data_read_2[1], data_read_2[2]);
    debug_printf("xCORE got %s, %d\n", ack_str(acks[TEST_WRITE_3]), n3);
  }

  // Done - stop test
  exit(0);
}

int main(void)
{
  i2c_master_if i2c[1];
  par {
    i2c_master_single_port(i2c, 1, p_i2c, SPEED, 1, 3, 0);
    {set_core_fast_mode_on(); test(i2c[0]);}

    // Keep the other cores busy
    par(int i=0;i<7;i++) while(1);
  }
  return 0;
}
