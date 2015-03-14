// Copyright (c) 2015, XMOS Ltd, All rights reserved
#include <stdio.h>
#include <xs1.h>
#include "debug_print.h"
#include "i2c.h"
#include <stdlib.h>
#include <timer.h>

port p_scl = XS1_PORT_1A;
port p_sda = XS1_PORT_1B;

void test(client i2c_master_async_if i2c)
{
  uint8_t data[3];
  int ack;
  size_t num_sent = 55;
  data[0] = 0x90; data[1] = 0xfe;
  i2c.write(0x3c, data, 2, 1);
  select {
  case i2c.operation_complete():
    ack = i2c.get_write_result(num_sent);
    break;
  }
  debug_printf("xCORE: %s. Transmitted %d bytes.\n",
               ack == I2C_SUCCEEDED ? "success" : "failure", num_sent);
  i2c.read(0x22, 2, 1);
  select {
  case i2c.operation_complete():
    ack = i2c.get_read_data(data, 2);
    break;
  }
  debug_printf("xCORE: %s\n",
               ack == I2C_SUCCEEDED ? "success" : "failure");
  debug_printf("xCORE received: 0x%x, 0x%x\n",data[0], data[1]);

  i2c.read(0x22, 1, 1);
  select {
  case i2c.operation_complete():
    ack = i2c.get_read_data(data, 1);
    break;
  }
  debug_printf("xCORE: %s\n",
               ack == I2C_SUCCEEDED ? "success" : "failure");
  debug_printf("xCORE received: 0x%x\n",data[0]);


  data[0] = 0xff; data[1] = 0x00; data[2] = 0xaa;
  i2c.write(0x7b, data, 3, 1);
  select {
  case i2c.operation_complete():
    ack = i2c.get_write_result(num_sent);
    break;
  }
  debug_printf("xCORE: %s. Transmitted %d bytes.\n",
               ack == I2C_SUCCEEDED ? "success" : "failure", num_sent);
  data[0] = 0xee;
  i2c.write(0x31, data, 1, 1);
  select {
  case i2c.operation_complete():
    ack = i2c.get_write_result(num_sent);
    break;
  }
  debug_printf("xCORE: %s. Transmitted %d bytes.\n",
               ack == I2C_SUCCEEDED ? "success" : "failure", num_sent);
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
    #ifdef COMB
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
