// Copyright (c) 2013-2016, XMOS Ltd, All rights reserved

#ifdef __XS2A__

#include "i2c.h"
#include <xs1.h>
#include <xclib.h>
#include <stdio.h>
#include <timer.h>
#include "xassert.h"
#include <print.h>

#define SDA_LOW     0
#define SCL_LOW     0

static const unsigned inline compute_low_period_time(unsigned bit_time)
{
  // Ensure the clock low period is longer than the high period
  return bit_time/2 + bit_time/32;
}

static const unsigned inline compute_bus_off_time(unsigned bit_time)
{
  return bit_time/2 + bit_time/32;
}

/** Reads back the SCL line, waiting until it goes high (in
 *  case the slave is clock stretching). It is assumed that the clock
 *  line has been release (driven high) before calling this function.
 *  Since the line going high may be delayed, the fall_time value may
 *  need to be adjusted
 */
static void wait_for_clock_high(port p_i2c,
                                   unsigned SCL_HIGH,
                                   unsigned &fall_time,
                                   unsigned delay)
{
  unsigned val = peek(p_i2c);
  while (!(val & SCL_HIGH)) {
    val = peek(p_i2c);
  }

  timer tmr;
  unsigned time;
  tmr when timerafter(fall_time + delay) :> time;

  // Adjust timing due to clock stretching without clock drift in the normal case
  if (time > fall_time + delay + 10) {
    fall_time = time - delay;
  }
}

static void high_pulse_drive(port p_i2c, int sdaValue, unsigned bit_time,
                             unsigned SCL_HIGH, unsigned SDA_HIGH,
                             unsigned S_REST,
                             unsigned &fall_time)
{
  timer tmr;
  sdaValue = sdaValue ? SDA_HIGH : SDA_LOW;
  p_i2c <: SCL_LOW  | sdaValue | S_REST;
  tmr when timerafter(fall_time + compute_low_period_time(bit_time)) :> void;
  p_i2c <: SCL_HIGH | sdaValue | S_REST;
  wait_for_clock_high(p_i2c, SCL_HIGH, fall_time, (bit_time * 3) / 4);
  fall_time = fall_time + bit_time;
  tmr when timerafter(fall_time) :> void;
  p_i2c <: SCL_LOW  | sdaValue | S_REST;
}

static int high_pulse_sample(port p_i2c, unsigned bit_time,
                             unsigned SCL_HIGH, unsigned SDA_HIGH,
                             unsigned S_REST,
                             unsigned &fall_time)
{
  timer tmr;

  p_i2c <: SCL_LOW | SDA_HIGH | S_REST;
  tmr when timerafter(fall_time + compute_low_period_time(bit_time)) :> void;
  p_i2c <: SCL_HIGH | SDA_HIGH | S_REST;
  wait_for_clock_high(p_i2c, SCL_HIGH, fall_time, (bit_time * 3) / 4);

  int sample_value = peek(p_i2c);
  if (sample_value & SDA_HIGH)
    sample_value = 1;
  else
    sample_value = 0;

  fall_time = fall_time + bit_time;
  tmr when timerafter(fall_time) :> void;
  p_i2c <: SCL_LOW | SDA_HIGH | S_REST;

  return sample_value;
}

static void start_bit(port p_i2c, unsigned bit_time,
                      unsigned SCL_HIGH, unsigned SDA_HIGH,
                      unsigned S_REST, unsigned &fall_time,
                      int stopped)
{
  timer tmr;

  if (!stopped) {
    tmr when timerafter(fall_time + compute_low_period_time(bit_time)) :> void;
    p_i2c <: SCL_HIGH | SDA_HIGH | S_REST;
    delay_ticks(compute_bus_off_time(bit_time));
  }

  p_i2c <: SCL_HIGH | SDA_LOW  | S_REST;
  delay_ticks(bit_time / 2);
  p_i2c <: SCL_LOW  | SDA_LOW  | S_REST;
  tmr :> fall_time;
}

static void stop_bit(port p_i2c, unsigned bit_time,
                     unsigned SCL_HIGH, unsigned SDA_HIGH,
                     unsigned S_REST, unsigned fall_time)
{
  timer tmr;

  // tmr when timerafter(fall_time + bit_time / 4) :> void;
  p_i2c <: SCL_LOW | SDA_LOW | S_REST;
  tmr when timerafter(fall_time + compute_low_period_time(bit_time)) :> void;
  p_i2c <: SCL_HIGH | SDA_LOW | S_REST;
  wait_for_clock_high(p_i2c, SCL_HIGH, fall_time, bit_time);
  p_i2c <: SCL_HIGH | SDA_HIGH | S_REST;
  delay_ticks(compute_bus_off_time(bit_time));
}

static int tx8(port p_i2c, unsigned data, unsigned bit_time,
                unsigned SCL_HIGH, unsigned SDA_HIGH,
                unsigned S_REST, unsigned &fall_time)
{
  unsigned bit_rev_data = ((unsigned) bitrev(data)) >> 24;
  for (int i = 8; i != 0; i--) {
    high_pulse_drive(p_i2c, bit_rev_data & 1, bit_time, SCL_HIGH, SDA_HIGH, S_REST, fall_time);
    bit_rev_data >>= 1;
  }
  return high_pulse_sample(p_i2c, bit_time, SCL_HIGH, SDA_HIGH, S_REST, fall_time);
}

[[distributable]]
void i2c_master_single_port(server interface i2c_master_if c[n], static const size_t n,
                            port p_i2c,
                            unsigned kbits_per_second,
                            unsigned scl_bit_position,
                            unsigned sda_bit_position,
                            unsigned other_bits_mask)
{
  unsigned last_fall_time = 0;
  unsigned bit_time = (XS1_TIMER_MHZ * 1000) / kbits_per_second;
  unsigned SDA_HIGH = (1 << sda_bit_position);
  unsigned SCL_HIGH = (1 << scl_bit_position);
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
      start_bit(p_i2c, bit_time, SCL_HIGH, SDA_HIGH, other_bits_mask, fall_time, stopped);
      int ack = tx8(p_i2c, (device << 1) | 1, bit_time, SCL_HIGH, SDA_HIGH, other_bits_mask, fall_time);
      if (ack == 0) {
        for (int j = 0; j < m; j++){
          unsigned char data = 0;
          timer tmr;
          for (int i = 8; i != 0; i--) {
            int temp = high_pulse_sample(p_i2c, bit_time, SCL_HIGH, SDA_HIGH, other_bits_mask, fall_time);
            data = (data << 1) | temp;
          }
          buf[j] = data;

          // ACK after every read byte until the final byte then NACK.
          unsigned sda = SDA_LOW;
          if (j == m-1) {
            sda = SDA_HIGH;
          }

          p_i2c <: SCL_LOW | sda | other_bits_mask;
          tmr when timerafter(fall_time + compute_low_period_time(bit_time)) :> void;
          p_i2c <: SCL_HIGH | sda | other_bits_mask;
          tmr when timerafter(fall_time + bit_time) :> void;

          // Release the data bus
          p_i2c <: SCL_LOW | SDA_HIGH | other_bits_mask;
          fall_time = fall_time + bit_time;
        }
      }
      if (send_stop_bit) {
        stop_bit(p_i2c, bit_time, SCL_HIGH, SDA_HIGH, other_bits_mask, fall_time);
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
      start_bit(p_i2c, bit_time, SCL_HIGH, SDA_HIGH, other_bits_mask, fall_time, stopped);
      int ack = tx8(p_i2c, device<<1, bit_time, SCL_HIGH, SDA_HIGH, other_bits_mask, fall_time);
      int j = 0;
      for (; j < n; j++) {
        if (ack != 0)
          break;

        ack = tx8(p_i2c, buf[j], bit_time, SCL_HIGH, SDA_HIGH, other_bits_mask, fall_time);
      }
      if (send_stop_bit) {
        stop_bit(p_i2c, bit_time, SCL_HIGH, SDA_HIGH, other_bits_mask, fall_time);
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
      stop_bit(p_i2c, bit_time, SCL_HIGH, SDA_HIGH, other_bits_mask, fall_time);
      locked_client = -1;
      break;

    case c[int i].shutdown():
      return;
    }
  }
}

#endif
