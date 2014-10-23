#include <stdio.h>
#include <xs1.h>
#include "debug_print.h"
#include "i2c.h"
#include <stdlib.h>

port p_scl = XS1_PORT_1A;
port p_sda = XS1_PORT_1B;

void test(client i2c_master_if i2c)
{
  i2c_res_t result;
  result = i2c.write_reg(0x44, 0x07, 0x12);
  debug_printf(result == I2C_SUCCEEDED ? "ACK\n" : "NACK\n");
  result = i2c.write_reg8_addr16(0x22, 0xfe99, 0x12);
  debug_printf(result == I2C_SUCCEEDED ? "ACK\n" : "NACK\n");
  result = i2c.write_reg16(0x33, 0xabcd, 0x12a3);
  debug_printf(result == I2C_SUCCEEDED ? "ACK\n" : "NACK\n");
  unsigned val;
  val = i2c.read_reg(0x44, 0x33, result);
  debug_printf(result == I2C_SUCCEEDED ? "ACK\n" : "NACK\n");
  debug_printf("val=%x\n", val);
  val = i2c.read_reg8_addr16(0x45, 0xa321, result);
  debug_printf(result == I2C_SUCCEEDED ? "ACK\n" : "NACK\n");
  debug_printf("val=%x\n", val);
  val = i2c.read_reg16(0x46, 0x3399, result);
  debug_printf(result == I2C_SUCCEEDED ? "ACK\n" : "NACK\n");
  debug_printf("val=%x\n", val);
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
