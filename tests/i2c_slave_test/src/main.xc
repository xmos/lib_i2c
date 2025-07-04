// Copyright 2014-2025 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <i2c.h>
#include <debug_print.h>
#include <xs1.h>
#include <syscall.h>
#include <print.h>

port p_scl = XS1_PORT_1A;
port p_sda = XS1_PORT_1B;

uint8_t test_data[] =
  {
    0xff,
    0x01,
    0x99,
    0x20,
    0x33,
    0xee
  };

[[distributable]]
void tester(server i2c_slave_callback_if i2c)
{
  int ack_sequence[7] = {I2C_SLAVE_ACK, I2C_SLAVE_ACK, I2C_SLAVE_NACK,
                         I2C_SLAVE_NACK,
                         I2C_SLAVE_ACK, I2C_SLAVE_NACK};
  int ack_index = 0;
  int i = 0;
  while (1) {
    select {
    case i2c.ack_read_request(void) -> i2c_slave_ack_t response:
      debug_printf("xCORE got start of read transaction\n");
      response = I2C_SLAVE_ACK;
      break;
    case i2c.ack_write_request(void) -> i2c_slave_ack_t response:
      debug_printf("xCORE got start of write transaction\n");
      response = I2C_SLAVE_ACK;
      break;
    case i2c.master_sent_data(uint8_t data) -> i2c_slave_ack_t response:
      debug_printf("xCORE got data: 0x%x\n", data);
      if (data == 0xff) {
        _exit(0);
      }
      response = ack_sequence[ack_index++];
      break;
    case i2c.master_requires_data() -> uint8_t data:
      data = test_data[i];
      debug_printf("xCORE sending: 0x%x\n", data);
      i++;
      if (i >= sizeof(test_data)) {
        i = 0;
      }
      break;
    case i2c.stop_bit():
      // The stop_bit function is timing critical. Needs to use printstr to meet
      // timing and detect the start bit
      printstr("xCORE got stop bit\n");
      break;
    }
  }
}

int main() {
  i2c_slave_callback_if i;
  par {
    tester(i);
    i2c_slave(i, p_scl, p_sda, 0x3c);
    par (int i = 0; i < 7;i++) {
      while (1);
    }
  }
  return 0;
}
