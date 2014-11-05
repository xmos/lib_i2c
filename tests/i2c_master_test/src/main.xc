#include <stdio.h>
#include <xs1.h>
#include "debug_print.h"
#include "i2c.h"
#include <stdlib.h>

port p_scl = XS1_PORT_1A;
port p_sda = XS1_PORT_1B;

void test(client i2c_master_if i2c)
{
  uint8_t data[3];
  int ack;
  size_t num_sent = 55;
  if (ENABLE_TX) {
    data[0] = 0x90; data[1] = 0xfe;
    ack = i2c.tx(0x3c, data, 2, num_sent, 1);
    debug_printf("xCORE got %s. Transmitted %d bytes.\n",
                 ack == I2C_SUCCEEDED ? "ack" : "nack", num_sent);
  }
  if (ENABLE_RX) {
    ack = i2c.rx(0x22, data, 2, 1);
    debug_printf("xCORE got %s\n",
                 ack == I2C_SUCCEEDED ? "ack" : "nack");
    debug_printf("xCORE received: 0x%x, 0x%x\n",data[0], data[1]);
    ack = i2c.rx(0x22, data, 1, 1);
    debug_printf("xCORE got %s\n",
                 ack == I2C_SUCCEEDED ? "ack" : "nack");
    debug_printf("xCORE received: 0x%x\n",data[0]);
  }
  if (ENABLE_TX) {
    data[0] = 0xff; data[1] = 0x00; data[2] = 0xaa;
    ack = i2c.tx(0x7b, data, 3, num_sent, 1);
    debug_printf("xCORE got %s. Transmitted %d bytes.\n",
                 ack == I2C_SUCCEEDED ? "ack" : "nack", num_sent);
    data[0] = 0xee;
    ack = i2c.tx(0x31, data, 1, num_sent, 1);
    debug_printf("xCORE got %s. Transmitted %d bytes.\n",
                 ack == I2C_SUCCEEDED ? "ack" : "nack", num_sent);
  }
  exit(0);
}

int main(void) {
  i2c_master_if i2c[1];
  par {
    i2c_master(i2c, 1, p_scl, p_sda, SPEED);
    {set_core_fast_mode_on();test(i2c[0]);}
    par(int i=0;i<7;i++) while(1);
  }
  return 0;
}
