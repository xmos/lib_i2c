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
  data[0] = 0x90; data[1] = 0xfe;
  i2c.tx(0x3c, data, 2);
  i2c.rx(0x22, data, 2);
  debug_printf("xCORE received: 0x%x, 0x%x\n",data[0], data[1]);
  i2c.rx(0x22, data, 1);
  debug_printf("xCORE received: 0x%x\n",data[0]);
  data[0] = 0xff; data[1] = 0x00; data[2] = 0xaa;
  i2c.tx(0x7b, data, 3);
  data[0] = 0xee;
  i2c.tx(0x31, data, 1);
  exit(0);
}

int main(void) {
  i2c_master_if i2c[1];
  par {
    i2c_master(i2c, 1, p_scl, p_sda, 400, I2C_ENABLE_MULTIMASTER);
    {set_core_fast_mode_on();test(i2c[0]);}
    par(int i=0;i<6;i++) while(1);
  }
  return 0;
}
