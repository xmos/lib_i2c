// Copyright (c) 2015-2018, XMOS Ltd, All rights reserved
#include <stdio.h>
#include <xs1.h>
#include "debug_print.h"
#include "i2c.h"
#include <stdlib.h>
#include <timer.h>

port p_scl = XS1_PORT_1A;
port p_sda = XS1_PORT_1B;

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

static const char * unsafe ack_str(int ack)
{
  unsafe {
    return (ack == I2C_ACK) ? "ack" : "nack";
  }
}

#define MAX_DATA_BYTES 3

void test(client i2c_master_async_if i2c)
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
  i2c.write(0x3c, data_write_1, 2, do_stop);
  select {
  case i2c.operation_complete():
    acks[TEST_WRITE_1] = i2c.get_write_result(n1);
    break;
  }

  i2c.read(0x22, 2, do_stop);
  select {
  case i2c.operation_complete():
    acks[TEST_READ_1] = i2c.get_read_data(data_read_1, 2);
    break;
  }

  i2c.read(0x22, 1, do_stop);
  select {
  case i2c.operation_complete():
    acks[TEST_READ_2] = i2c.get_read_data(data_read_2, 1);
    break;
  }

  i2c.write(0x7b, data_write_2, 3, do_stop);
  select {
  case i2c.operation_complete():
    acks[TEST_WRITE_2] = i2c.get_write_result(n2);
    break;
  }

  i2c.write(0x31, data_write_3, 1, do_stop);
  select {
  case i2c.operation_complete():
    acks[TEST_WRITE_3] = i2c.get_write_result(n3);
    break;
  }

  // Print out results after all the data transactions have finished
  unsafe {
    debug_printf("xCORE got %s, %d\n", ack_str(acks[TEST_WRITE_1]), n1);
    debug_printf("xCORE got %s, %d\n", ack_str(acks[TEST_WRITE_2]), n2);
    debug_printf("xCORE got %s, %d\n", ack_str(acks[TEST_WRITE_3]), n3);

    debug_printf("xCORE got %s\n", ack_str(acks[TEST_READ_1]));
    debug_printf("xCORE received: 0x%x, 0x%x\n", data_read_1[0], data_read_1[1]);
    debug_printf("xCORE got %s\n", ack_str(acks[TEST_READ_2]));
    debug_printf("xCORE received: 0x%x\n", data_read_2[0]);
  }
  exit(0);
}

/** This combinable task will randomly interrupt the i2c master combinable
 *  task and add delays. This should slow down the speed but should still
 *  produce valid i2c transactions.
 */
[[combinable]]
void interference()
{
  timer tmr;
  unsigned timeout;
  tmr :> timeout;
  while (1) {
    select {
    case tmr when timerafter(timeout) :> void:
      int delay = rand() >> 20;
      delay_ticks(delay);
      tmr :> timeout;
      timeout += rand() >> 20;
      break;
    }
  }
}

int main(void) {
  i2c_master_async_if i2c[1];
  par {
    #if COMB
      #ifdef INTERFERE
      [[combine]]
      par {
        i2c_master_async_comb(i2c, 1, p_scl, p_sda, SPEED, 10);
        interference();
      }
      #else
      i2c_master_async_comb(i2c, 1, p_scl, p_sda, SPEED, 10);
      #endif
    #else
      i2c_master_async(i2c, 1, p_scl, p_sda, SPEED, 10);
    #endif
    {set_core_fast_mode_on();test(i2c[0]);}
    par(int i=0;i<6;i++) while(1);
  }
  return 0;
}
