// Copyright (c) 2013-2021, XMOS Ltd, All rights reserved
#include <xs1.h>
#include "i2c.h"
#include "debug_print.h"

// I2C interface ports
port p_scl = XS1_PORT_1E;
port p_sda = XS1_PORT_1F;

// FXOS8700CQ register address defines
#define FXOS8700CQ_I2C_ADDR 0x1E
#define FXOS8700CQ_XYZ_DATA_CFG_REG 0x0E
#define FXOS8700CQ_CTRL_REG_1 0x2A
#define FXOS8700CQ_DR_STATUS 0x0
#define FXOS8700CQ_OUT_X_MSB 0x1
#define FXOS8700CQ_OUT_Y_MSB 0x3
#define FXOS8700CQ_OUT_Z_MSB 0x5

int read_acceleration(client interface i2c_master_if i2c, int axis) {
    i2c_regop_res_t result;

    uint8_t msb_data = i2c.read_reg(FXOS8700CQ_I2C_ADDR, axis, result);
    if (result != I2C_REGOP_SUCCESS) {
      debug_printf("I2C read reg failed\n");
      return 0;
    }

    uint8_t lsb_data = i2c.read_reg(FXOS8700CQ_I2C_ADDR, axis+1, result);
    if (result != I2C_REGOP_SUCCESS) {
      debug_printf("I2C read reg failed\n");
      return 0;
    }

    int accel_val = (msb_data << 2) | (lsb_data >> 6);
    if (accel_val & 0x200) {
      accel_val -= 1023;
    }
    return accel_val;
}

void accelerometer(client interface i2c_master_if i2c) {
  i2c_regop_res_t result;

  // Configure FXOS8700CQ
  result = i2c.write_reg(FXOS8700CQ_I2C_ADDR, FXOS8700CQ_XYZ_DATA_CFG_REG, 0x01);

  if (result != I2C_REGOP_SUCCESS) {
    debug_printf("I2C write reg failed\n");
  }

  // Enable FXOS8700CQ
  result = i2c.write_reg(FXOS8700CQ_I2C_ADDR, FXOS8700CQ_CTRL_REG_1, 0x01);
  if (result != I2C_REGOP_SUCCESS) {
    debug_printf("I2C write reg failed\n");
  }

  while (1) {
    // Wait for data ready from FXOS8700CQ
    char status_data = 0;
    do {
      status_data = i2c.read_reg(FXOS8700CQ_I2C_ADDR, FXOS8700CQ_DR_STATUS, result);
    } while (!status_data & 0x08);

    int x = read_acceleration(i2c, FXOS8700CQ_OUT_X_MSB);
    int y = read_acceleration(i2c, FXOS8700CQ_OUT_Y_MSB);
    int z = read_acceleration(i2c, FXOS8700CQ_OUT_Z_MSB);

    debug_printf("X = %d, Y = %d, Z = %d       \r", x, y, z);
  }
  // End accelerometer
}

int main(void) {
  i2c_master_if i2c[1];
  par {
    i2c_master(i2c, 1, p_scl, p_sda, 10);
    accelerometer(i2c[0]);
  }
  return 0;
}
