// Copyright 2025 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <i2c.h>
#include <xs1.h>
#include <xclib.h>
#include <timer.h>

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
  port p_sda)
{
  timer tmr;
  unsigned start_time, timeout_time;
  const unsigned PULLUP_TIMEOUT_TICKS = 1000; // 10us timeout for pull-up detection
  
  // Test SCL pull-up
  p_scl <: 0; // Drive SCL low
  delay_ticks(100); // Brief delay to ensure line is driven low
  p_scl :> void; // Release SCL
  
  tmr :> start_time;
  timeout_time = start_time + PULLUP_TIMEOUT_TICKS;
  
  // Use select with timeout to avoid hanging
  select {
    case p_scl when pinseq(1) :> void:
      // SCL pull-up is working
      break;
    case tmr when timerafter(timeout_time) :> void:
      return I2C_SCL_PULLUP_MISSING;
  }
  
  // Test SDA pull-up
  p_sda <: 0; // Drive SDA low
  delay_ticks(100); // Brief delay to ensure line is driven low
  p_sda :> void; // Release SDA
  
  tmr :> start_time;
  timeout_time = start_time + PULLUP_TIMEOUT_TICKS;
  
  // Use select with timeout to avoid hanging
  select {
    case p_sda when pinseq(1) :> void:
      // SDA pull-up is working
      break;
    case tmr when timerafter(timeout_time) :> void:
      return I2C_SDA_PULLUP_MISSING;
  }
  
  return I2C_ACK;
}

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
  static const unsigned other_bits_mask)
{
  const unsigned SCL_HIGH = BIT_MASK(scl_bit_position);
  const unsigned SDA_HIGH = BIT_MASK(sda_bit_position);
  const unsigned SCL_LOW = 0;
  const unsigned SDA_LOW = 0;
  
  timer tmr;
  unsigned start_time, timeout_time;
  const unsigned PULLUP_TIMEOUT_TICKS = 1000; // 10us timeout for pull-up detection
  
  // Test SCL pull-up
  p_i2c <: SCL_LOW | SDA_HIGH | other_bits_mask; // Drive SCL low, SDA high
  delay_ticks(100); // Brief delay to ensure line is driven low
  p_i2c <: SCL_HIGH | SDA_HIGH | other_bits_mask; // Release SCL
  
  tmr :> start_time;
  timeout_time = start_time + PULLUP_TIMEOUT_TICKS;
  
  unsigned val = peek(p_i2c);
  while (!(val & SCL_HIGH)) {
    tmr :> start_time;
    if (start_time > timeout_time) {
      return I2C_SCL_PULLUP_MISSING;
    }
    val = peek(p_i2c);
  }
  
  // Test SDA pull-up
  p_i2c <: SCL_HIGH | SDA_LOW | other_bits_mask; // Drive SDA low, SCL high
  delay_ticks(100); // Brief delay to ensure line is driven low
  p_i2c <: SCL_HIGH | SDA_HIGH | other_bits_mask; // Release SDA
  
  tmr :> start_time;
  timeout_time = start_time + PULLUP_TIMEOUT_TICKS;
  
  val = peek(p_i2c);
  while (!(val & SDA_HIGH)) {
    tmr :> start_time;
    if (start_time > timeout_time) {
      return I2C_SDA_PULLUP_MISSING;
    }
    val = peek(p_i2c);
  }
  
  return I2C_ACK;
}