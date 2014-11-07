#include <stdio.h>
#include <xs1.h>
#include "debug_print.h"
#include "i2c.h"
#include <stdlib.h>

port p_i2c = XS1_PORT_8A;

void test(client i2c_master_if i2c)
{
  uint8_t data[3];
  int ack;
  size_t n;
  data[0] = 0x90; data[1] = 0xfe;
  ack = i2c.write(0x3c, data, 2, n, 1);
  debug_printf("xCORE got %s\n",
               ack == I2C_SUCCEEDED ? "ack" : "nack");
  data[0] = 0xff; data[1] = 0x00; data[2] = 0xaa;
  ack = i2c.write(0x7b, data, 3, n, 1);
  debug_printf("xCORE got %s\n",
               ack == I2C_SUCCEEDED ? "ack" : "nack");
  data[0] = 0xee;
  ack = i2c.write(0x31, data, 1, n, 1);
  debug_printf("xCORE got %s\n",
               ack == I2C_SUCCEEDED ? "ack" : "nack");
  exit(0);
}

int main(void) {
  i2c_master_if i2c[1];
  par {
    i2c_master_single_port(i2c, 1, p_i2c, SPEED, 1, 3, 0);
    {set_core_fast_mode_on();test(i2c[0]);}
    par(int i=0;i<7;i++) while(1);
  }
  return 0;
}
