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
  if (ENABLE_TX) {
    data[0] = 0x90; data[1] = 0xfe;
    ack = i2c.tx(0x3c, data, 2);
    debug_printf("xCORE got %s\n",
                 ack == I2C_WRITE_ACK_SUCCEEDED ? "ack" : "nack");
  }
  if (ENABLE_RX) {
    i2c.rx(0x22, data, 2);
    debug_printf("xCORE received: 0x%x, 0x%x\n",data[0], data[1]);
    i2c.rx(0x22, data, 1);
    debug_printf("xCORE received: 0x%x\n",data[0]);
  }
  if (ENABLE_TX) {
    data[0] = 0xff; data[1] = 0x00; data[2] = 0xaa;
    ack = i2c.tx(0x7b, data, 3);
    debug_printf("xCORE got %s\n",
                 ack == I2C_WRITE_ACK_SUCCEEDED ? "ack" : "nack");
    data[0] = 0xee;
    ack = i2c.tx(0x31, data, 1);
    debug_printf("xCORE got %s\n",
                 ack == I2C_WRITE_ACK_SUCCEEDED ? "ack" : "nack");
  }
  exit(0);
}

int main(void) {
  i2c_master_if i2c[1];
  par {
    i2c_master(i2c, 1, p_scl, p_sda, SPEED, I2C_ENABLE_MULTIMASTER);
    {set_core_fast_mode_on();test(i2c[0]);}
    par(int i=0;i<7;i++) while(1);
  }
  return 0;
}
