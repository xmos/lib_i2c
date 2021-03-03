// Copyright (c) 2013-2021, XMOS Ltd, All rights reserved
// This software is available under the terms provided in LICENSE.txt.
#if (defined(__XS2A__) || defined(__XS3A__))

#include "i2c.h"
#include <xs1.h>
#include <xclib.h>
#include <stdio.h>
#include <timer.h>

#include "xassert.h"

#define SDA_LOW     0
#define SCL_LOW     0

/* NOTE: the kbits_per_second needs to be passed around due to the fact that the
 *       compiler won't compute a new static const from a static const.
 */

/** Return the number of 10ns timer ticks required to meet the timing as defined
 *  in the standards.
 */
static const unsigned inline compute_low_period_ticks(
  static const unsigned kbits_per_second)
{
  unsigned ticks = 0;
  if (kbits_per_second <= 100) {
    const unsigned four_point_seven_micro_seconds_in_ticks = 470;
    ticks = four_point_seven_micro_seconds_in_ticks;
  } else if (kbits_per_second <= 400) {
    const unsigned one_point_three_micro_seconds_in_ticks = 130;
    ticks = one_point_three_micro_seconds_in_ticks;
  } else {
    fail("Fast-mode Plus not implemented");
  }

  // There is some jitter on the falling edges of the clock. In order to ensure
  // that the low period is respected we need to extend the minimum low period.
  const unsigned jitter_ticks = 3;
  return ticks + jitter_ticks;
}

static const unsigned inline compute_bus_off_ticks(
  static const unsigned kbits_per_second)
{
  const unsigned bit_time = BIT_TIME(kbits_per_second);

  // Ensure the bus off time is respected. This is just over 1/2 bit time in
  // the case of the Fast-mode I2C so adding bit_time/16 ensures the timing
  // will be enforced
  return bit_time/2 + bit_time/16;
}

/** Reads back the SCL line, waiting until it goes high (in
 *  case the slave is clock stretching). It is assumed that the clock
 *  line has been release (driven high) before calling this function.
 *  Since the line going high may be delayed, the fall_time value may
 *  need to be adjusted
 */
static void wait_for_clock_high(
  port p_i2c,
  static const unsigned scl_bit_position,
  unsigned &fall_time,
  unsigned delay)
{
  const unsigned SCL_HIGH = BIT_MASK(scl_bit_position);

  unsigned val = peek(p_i2c);
  while (!(val & SCL_HIGH)) {
    val = peek(p_i2c);
  }

  timer tmr;
  unsigned time;
  tmr when timerafter(fall_time + delay) :> time;

  // Adjust timing due to support clock stretching without clock drift in the
  // normal case.

  // If the time is beyond the time it takes simply to wake up and start
  // executing then the clock needs to be adjusted
  const int wake_up_ticks = 10;
  if (time > fall_time + delay + wake_up_ticks) {
    fall_time = time - delay - wake_up_ticks;
  }
}

static void high_pulse_drive(
  port p_i2c,
  int sdaValue,
  static const unsigned kbits_per_second,
  static const unsigned scl_bit_position,
  static const unsigned sda_bit_position,
  static const unsigned other_bits_mask,
  unsigned &fall_time)
{
  const unsigned bit_time = BIT_TIME(kbits_per_second);
  const unsigned SCL_HIGH = BIT_MASK(scl_bit_position);
  const unsigned SDA_HIGH = BIT_MASK(sda_bit_position);

  timer tmr;
  sdaValue = sdaValue ? SDA_HIGH : SDA_LOW;
  p_i2c <: SCL_LOW  | sdaValue | other_bits_mask;
  tmr when timerafter(fall_time + compute_low_period_ticks(kbits_per_second)) :> void;
  p_i2c <: SCL_HIGH | sdaValue | other_bits_mask;
  wait_for_clock_high(p_i2c, scl_bit_position, fall_time, (bit_time * 3) / 4);
  fall_time = fall_time + bit_time;
  tmr when timerafter(fall_time) :> void;
  p_i2c <: SCL_LOW  | sdaValue | other_bits_mask;
}

static int high_pulse_sample(
  port p_i2c,
  static const unsigned kbits_per_second,
  static const unsigned scl_bit_position,
  static const unsigned sda_bit_position,
  static const unsigned other_bits_mask,
  unsigned &fall_time)
{
  const unsigned bit_time = BIT_TIME(kbits_per_second);
  const unsigned SCL_HIGH = BIT_MASK(scl_bit_position);
  const unsigned SDA_HIGH = BIT_MASK(sda_bit_position);

  timer tmr;

  p_i2c <: SCL_LOW | SDA_HIGH | other_bits_mask;
  tmr when timerafter(fall_time + compute_low_period_ticks(kbits_per_second)) :> void;
  p_i2c <: SCL_HIGH | SDA_HIGH | other_bits_mask;
  wait_for_clock_high(p_i2c, scl_bit_position, fall_time, (bit_time * 3) / 4);

  int sample_value = peek(p_i2c);
  if (sample_value & SDA_HIGH)
    sample_value = 1;
  else
    sample_value = 0;

  fall_time = fall_time + bit_time;
  tmr when timerafter(fall_time) :> void;
  p_i2c <: SCL_LOW | SDA_HIGH | other_bits_mask;

  return sample_value;
}

static void start_bit(
  port p_i2c,
  static const unsigned kbits_per_second,
  static const unsigned scl_bit_position,
  static const unsigned sda_bit_position,
  static const unsigned other_bits_mask,
  unsigned &fall_time,
  int stopped)
{
  const unsigned bit_time = BIT_TIME(kbits_per_second);
  const unsigned SCL_HIGH = BIT_MASK(scl_bit_position);
  const unsigned SDA_HIGH = BIT_MASK(sda_bit_position);

  timer tmr;

  if (!stopped) {
    tmr when timerafter(fall_time + compute_low_period_ticks(kbits_per_second)) :> void;
    p_i2c <: SCL_HIGH | SDA_HIGH | other_bits_mask;
    wait_for_clock_high(p_i2c, scl_bit_position, fall_time, compute_bus_off_ticks(kbits_per_second));
  }

  p_i2c <: SCL_HIGH | SDA_LOW  | other_bits_mask;
  delay_ticks(bit_time / 2);
  p_i2c <: SCL_LOW  | SDA_LOW  | other_bits_mask;
  tmr :> fall_time;
}

static void stop_bit(
  port p_i2c,
  static const unsigned kbits_per_second,
  static const unsigned scl_bit_position,
  static const unsigned sda_bit_position,
  static const unsigned other_bits_mask,
  unsigned fall_time)
{
  const unsigned bit_time = BIT_TIME(kbits_per_second);
  const unsigned SCL_HIGH = BIT_MASK(scl_bit_position);
  const unsigned SDA_HIGH = BIT_MASK(sda_bit_position);

  timer tmr;

  p_i2c <: SCL_LOW | SDA_LOW | other_bits_mask;
  tmr when timerafter(fall_time + compute_low_period_ticks(kbits_per_second)) :> void;
  p_i2c <: SCL_HIGH | SDA_LOW | other_bits_mask;
  wait_for_clock_high(p_i2c, scl_bit_position, fall_time, bit_time);
  p_i2c <: SCL_HIGH | SDA_HIGH | other_bits_mask;
  delay_ticks(compute_bus_off_ticks(kbits_per_second));
}

static int tx8(
  port p_i2c, unsigned data,
  static const unsigned kbits_per_second,
  static const unsigned scl_bit_position,
  static const unsigned sda_bit_position,
  static const unsigned other_bits_mask,
  unsigned &fall_time)
{
  unsigned bit_rev_data = ((unsigned) bitrev(data)) >> 24;
  for (int i = 8; i != 0; i--) {
    high_pulse_drive(p_i2c, bit_rev_data & 1, kbits_per_second, scl_bit_position, sda_bit_position, other_bits_mask, fall_time);
    bit_rev_data >>= 1;
  }
  return high_pulse_sample(p_i2c, kbits_per_second, scl_bit_position, sda_bit_position, other_bits_mask, fall_time);
}

[[distributable]]
void i2c_master_single_port(
  server interface i2c_master_if c[n],
  static const size_t n,
  port p_i2c,
  static const unsigned kbits_per_second,
  static const unsigned scl_bit_position,
  static const unsigned sda_bit_position,
  static const unsigned other_bits_mask)
{
  const unsigned bit_time = BIT_TIME(kbits_per_second);
  const unsigned SCL_HIGH = BIT_MASK(scl_bit_position);
  const unsigned SDA_HIGH = BIT_MASK(sda_bit_position);

  unsigned last_fall_time = 0;
  unsigned locked_client = -1;
  set_port_drive_low(p_i2c);
  p_i2c <: SCL_HIGH | SDA_HIGH | other_bits_mask;

  while (1) {
    select {
    case (size_t i =0; i < n; i++)
      (n == 1 || (locked_client == -1 || i == locked_client)) =>
      c[i].read(uint8_t device, uint8_t buf[m], size_t m,
              int send_stop_bit) -> i2c_res_t result:

      const int stopped = locked_client == -1;
      unsigned fall_time = last_fall_time;
      start_bit(p_i2c, kbits_per_second, scl_bit_position, sda_bit_position, other_bits_mask, fall_time, stopped);
      int ack = tx8(p_i2c, (device << 1) | 1, kbits_per_second, scl_bit_position, sda_bit_position, other_bits_mask, fall_time);
      if (ack == 0) {
        for (int j = 0; j < m; j++){
          unsigned char data = 0;
          timer tmr;
          for (int i = 8; i != 0; i--) {
            int temp = high_pulse_sample(p_i2c, kbits_per_second, scl_bit_position, sda_bit_position, other_bits_mask, fall_time);
            data = (data << 1) | temp;
          }
          buf[j] = data;

          // ACK after every read byte until the final byte then NACK.
          unsigned sda = SDA_LOW;
          if (j == m-1) {
            sda = SDA_HIGH;
          }

          p_i2c <: SCL_LOW | sda | other_bits_mask;
          tmr when timerafter(fall_time + compute_low_period_ticks(kbits_per_second)) :> void;
          p_i2c <: SCL_HIGH | sda | other_bits_mask;
          wait_for_clock_high(p_i2c, scl_bit_position, fall_time, (bit_time * 3) / 4);
          fall_time = fall_time + bit_time;
          tmr when timerafter(fall_time) :> void;

          // Release the data bus
          p_i2c <: SCL_LOW | SDA_HIGH | other_bits_mask;
        }
      }
      if (send_stop_bit) {
        stop_bit(p_i2c, kbits_per_second, scl_bit_position, sda_bit_position, other_bits_mask, fall_time);
        locked_client = -1;
      } else {
        locked_client = i;
      }

      result = (ack == 0) ? I2C_ACK : I2C_NACK;

      // Remember the last fall time to ensure the next start bit is valid
      last_fall_time = fall_time;
      break;

    case (size_t i = 0; i < n; i++)
      (n == 1 || (locked_client == -1 || i == locked_client)) =>
        c[i].write(uint8_t device, uint8_t buf[n], size_t n,
                size_t &num_bytes_sent,
                int send_stop_bit) -> i2c_res_t result:

      const int stopped = locked_client == -1;
      unsigned fall_time = last_fall_time;
      start_bit(p_i2c, kbits_per_second, scl_bit_position, sda_bit_position, other_bits_mask, fall_time, stopped);
      int ack = tx8(p_i2c, device<<1, kbits_per_second, scl_bit_position, sda_bit_position, other_bits_mask, fall_time);
      int j = 0;
      for (; j < n; j++) {
        if (ack != 0)
          break;

        ack = tx8(p_i2c, buf[j], kbits_per_second, scl_bit_position, sda_bit_position, other_bits_mask, fall_time);
      }
      if (send_stop_bit) {
        stop_bit(p_i2c, kbits_per_second, scl_bit_position, sda_bit_position, other_bits_mask, fall_time);
        locked_client = -1;
      } else {
        locked_client = i;
      }
      num_bytes_sent = j;
      result = (ack == 0) ? I2C_ACK : I2C_NACK;

      // Remember the last fall time to ensure the next start bit is valid
      last_fall_time = fall_time;
      break;

    case c[int i].send_stop_bit(void):
      timer tmr;
      unsigned fall_time;
      tmr :> fall_time;
      stop_bit(p_i2c, kbits_per_second, scl_bit_position, sda_bit_position, other_bits_mask, fall_time);
      locked_client = -1;
      break;

    case c[int i].shutdown():
      return;
    }
  }
}

#endif
