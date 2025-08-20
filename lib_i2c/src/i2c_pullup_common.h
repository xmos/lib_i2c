// Copyright 2025 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#ifndef _i2c_pullup_common_h_
#define _i2c_pullup_common_h_

#include <i2c.h>
#include <xs1.h>

/** Check if pull-up resistors are present on SCL and SDA lines for two-port I2C.
 *  This function drives the lines low briefly, then releases them
 *  and checks if they return to high state within a reasonable time.
 *  
 *  \param p_scl       SCL port
 *  \param p_sda       SDA port
 *  
 *  \returns I2C_ACK if both pull-ups are present,
 *           I2C_SCL_PULLUP_MISSING if SCL pull-up is missing,
 *           I2C_SDA_PULLUP_MISSING if SDA pull-up is missing
 */
i2c_res_t check_pullups_two_port(
  port p_scl,
  port p_sda);

/** Check if pull-up resistors are present on SCL and SDA lines for single-port I2C.
 *  This function drives the lines low briefly, then releases them
 *  and checks if they return to high state within a reasonable time.
 *  
 *  \param p_i2c              single port containing both SCL and SDA
 *  \param scl_bit_position   bit position of SCL on the port
 *  \param sda_bit_position   bit position of SDA on the port
 *  \param other_bits_mask    mask for other bits that should be preserved
 *  
 *  \returns I2C_ACK if both pull-ups are present,
 *           I2C_SCL_PULLUP_MISSING if SCL pull-up is missing,
 *           I2C_SDA_PULLUP_MISSING if SDA pull-up is missing
 */
i2c_res_t check_pullups_single_port(
  port p_i2c,
  static const unsigned scl_bit_position,
  static const unsigned sda_bit_position,
  static const unsigned other_bits_mask);

#endif // _i2c_pullup_common_h_