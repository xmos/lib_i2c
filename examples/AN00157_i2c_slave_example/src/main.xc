// Copyright 2014-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <xs1.h>
#include <syscall.h>

#include "i2c.h"
#include "debug_print.h"

port p_slave_scl = XS1_PORT_1E;
port p_slave_sda = XS1_PORT_1F;

port p_master_scl = XS1_PORT_1G;
port p_master_sda = XS1_PORT_1H;

/*
 * Interface definition between user application and I2C slave register file
 */
typedef interface register_if {
  /* Set a register value
   */
  void set_register(int regnum, uint8_t data);

  /* Get a register value.
   */
  uint8_t get_register(int regnum);

  /* Get the number of the register that has changed
   * This will also clear the notification
   */
  [[clears_notification]]
  unsigned get_changed_regnum();

  /* Notification from the register file to the application that a register
   * value has changed
   */
  [[notification]]
  slave void register_changed();
} register_if;

#define NUM_REGISTERS 10

[[distributable]]
void i2c_slave_register_file(server i2c_slave_callback_if i2c,
                             server register_if app)
{
  uint8_t registers[NUM_REGISTERS];

  // This variable is set to -1 if no current register has been selected.
  // If the I2C master does a write transaction to select the register then
  // the variable will be updated to the register the master wants to
  // read/update.
  int current_regnum = -1;
  int changed_regnum = -1;
  while (1) {
    select {

    // Handle application requests to get/set register values.
    case app.set_register(int regnum, uint8_t data):
      if (regnum >= 0 && regnum < NUM_REGISTERS) {
        registers[regnum] = data;
      }
      break;
    case app.get_register(int regnum) -> uint8_t data:
      if (regnum >= 0 && regnum < NUM_REGISTERS) {
        data = registers[regnum];
      } else {
        data = 0;
      }
      break;
    case app.get_changed_regnum() -> unsigned regnum:
      regnum = changed_regnum;
      break;

    // Handle I2C slave transactions
    case i2c.ack_read_request(void) -> i2c_slave_ack_t response:
      // If no register has been selected using a previous write
      // transaction the NACK, otherwise ACK
      if (current_regnum == -1) {
        response = I2C_SLAVE_NACK;
      } else {
        response = I2C_SLAVE_ACK;
      }
      break;
    case i2c.ack_write_request(void) -> i2c_slave_ack_t response:
      // Write requests are always accepted
      response = I2C_SLAVE_ACK;
      break;
    case i2c.master_sent_data(uint8_t data) -> i2c_slave_ack_t response:
      // The master is trying to write, which will either select a register
      // or write to a previously selected register
      if (current_regnum != -1) {
        registers[current_regnum] = data;
        debug_printf("REGFILE: reg[%d] <- %x\n", current_regnum, data);

        // Inform the user application that the register has changed
        changed_regnum = current_regnum;
        app.register_changed();

        response = I2C_SLAVE_ACK;
      }
      else {
        if (data < NUM_REGISTERS) {
          current_regnum = data;
          debug_printf("REGFILE: select reg[%d]\n", current_regnum);
          response = I2C_SLAVE_ACK;
        } else {
          response = I2C_SLAVE_NACK;
        }
      }
      break;
    case i2c.master_requires_data() -> uint8_t data:
      // The master is trying to read, if a register is selected then
      // return the value (other return 0).
      if (current_regnum != -1) {
        data = registers[current_regnum];
        debug_printf("REGFILE: reg[%d] -> %x\n", current_regnum, data);
      } else {
        data = 0;
      }
      break;
    case i2c.stop_bit():
      // The I2C transaction has completed, clear the regnum
      debug_printf("REGFILE: stop_bit\n");
      current_regnum = -1;
      break;
    } // select
  }
}

void slave_application(client register_if reg)
{
  // Invert the data of any register that is written
  while (1) {
    select {
      case reg.register_changed():
        unsigned regnum = reg.get_changed_regnum();
        unsigned value = reg.get_register(regnum);
        debug_printf("SLAVE: Change register %d value from %x to %x\n",
          regnum, value, ~value & 0xff);
        reg.set_register(regnum, ~value);
        break;
    }
  }
}

void master_application(client interface i2c_master_if i2c, uint8_t device_addr)
{
  i2c_regop_res_t reg_result;

  // Write a single register
  reg_result = i2c.write_reg(device_addr, 0x03, 0x12);
  if (reg_result != I2C_REGOP_SUCCESS) {
    debug_printf("Write reg 0x03 failed!\n");
  }

  // Read a single register and check the result
  uint8_t data = i2c.read_reg(device_addr, 0x03, reg_result);
  if (reg_result != I2C_REGOP_SUCCESS) {
    debug_printf("Read reg 0x03 failed!\n");
  }
  debug_printf("MASTER: Read from addr 0x%x, 0x%x %s (got 0x%x, expected 0x%x)\n",
    device_addr, 0x03, (data == 0xed) ? "SUCCESS" : "FAILED", data, 0xed);

  // Test finished
  _exit(0);
}

int main() {
  i2c_slave_callback_if i_i2c;
  register_if i_reg;
  i2c_master_if i2c[1];
  uint8_t device_addr = 0x3c;

  par {
    i2c_slave_register_file(i_i2c, i_reg);
    i2c_slave(i_i2c, p_slave_scl, p_slave_sda, device_addr);
    slave_application(i_reg);

    i2c_master(i2c, 1, p_master_scl, p_master_sda, 200);
    master_application(i2c[0], device_addr);
  }
  return 0;
}
