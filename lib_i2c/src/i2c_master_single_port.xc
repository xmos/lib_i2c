// Copyright (c) 2015, XMOS Ltd, All rights reserved

#include "i2c.h"
#include <xs1.h>
#include <xclib.h>
#include <stdio.h>
#include <timer.h>
#include "xassert.h"
#include <print.h>

#define SDA_LOW     0
#define SCL_LOW     0

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
  timer tmr;
  unsigned time;
  unsigned val;
  val = peek(p_i2c);
  while (!(val & SCL_HIGH)) {
    val = peek(p_i2c);
  }
  tmr when timerafter(fall_time + delay) :> time;
  // Adjust timing due to clock stretching
  fall_time = time - delay;
}

static void high_pulse_drive(port p_i2c, int sdaValue, unsigned bit_time,
                             unsigned SCL_HIGH, unsigned SDA_HIGH,
                             unsigned S_REST,
                             unsigned &fall_time)
{
  #ifdef __XS2A__
  timer tmr;
  unsigned val;
  if (sdaValue)
    val = SDA_HIGH | SCL_LOW | S_REST;
  else
    val = SDA_LOW  | SCL_LOW | S_REST;
  p_i2c <: val;
  tmr when timerafter(fall_time + bit_time / 2 + bit_time / 32) :> void;
  if (sdaValue)
    val = SDA_HIGH | SCL_HIGH | S_REST;
  else
    val = SDA_LOW  | SCL_HIGH | S_REST;
  p_i2c <: val;
  wait_for_clock_high(p_i2c, SCL_HIGH, fall_time, (bit_time * 3) / 4);
  fall_time = fall_time + bit_time;
  tmr when timerafter(fall_time) :> void;
  if (sdaValue)
    val = SDA_HIGH | SCL_LOW | S_REST;
  else
    val = SDA_LOW  | SCL_LOW | S_REST;
  p_i2c <: val;
  #else
  timer tmr;
  if (sdaValue) {
    p_i2c <: SDA_HIGH | SCL_LOW | S_REST;
    tmr when timerafter(fall_time + bit_time / 2 + bit_time / 32) :> void;
    p_i2c <: SDA_HIGH | SCL_HIGH | S_REST;
    fall_time += bit_time;
    tmr when timerafter(fall_time) :> void;
    p_i2c <: SDA_HIGH | SCL_LOW | S_REST;
  } else {
    p_i2c <: SDA_LOW | SCL_LOW | S_REST;
    tmr when timerafter(fall_time + bit_time / 2 + bit_time / 32) :> void;
    p_i2c <: SDA_LOW | SCL_HIGH | S_REST;
    fall_time += bit_time;
    tmr when timerafter(fall_time) :> void;
    p_i2c <: SDA_LOW | SCL_LOW | S_REST;
  }
  #endif
}

static int high_pulse_sample(port p_i2c, unsigned bit_time,
                             unsigned SCL_HIGH, unsigned SDA_HIGH,
                             unsigned S_REST,
                             unsigned &fall_time)
{
  #ifdef __XS2A__
  int sample_value;
  timer tmr;

  p_i2c <: SCL_LOW | SDA_HIGH | S_REST;
  tmr when timerafter(fall_time + bit_time / 2 + bit_time / 32) :> void;
  p_i2c <: SCL_HIGH | SDA_HIGH | S_REST;
  wait_for_clock_high(p_i2c, SCL_HIGH, fall_time, (bit_time * 3) / 4);
  sample_value = peek(p_i2c);
  if (sample_value & SDA_HIGH)
    sample_value = 1;
  else
    sample_value = 0;

  fall_time = fall_time + bit_time;
  tmr when timerafter(fall_time) :> void;
  p_i2c <: SCL_LOW | SDA_HIGH;
  return sample_value;
  #else
  int expectedSDA = 0;
  timer tmr;
  p_i2c <: (expectedSDA ? SDA_HIGH : 0) | SCL_LOW | S_REST;
  tmr when timerafter(fall_time + bit_time / 2 + bit_time / 32) :> void;
  p_i2c :> void;
  tmr when timerafter(fall_time + (bit_time * 3) / 4) :> void;
  expectedSDA = peek(p_i2c) & SDA_HIGH;
  fall_time += bit_time + 1;
  tmr when timerafter(fall_time) :> void;
  p_i2c <: expectedSDA | SCL_LOW | S_REST;
  return expectedSDA;
  #endif
}

static unsigned start_bit(port p_i2c, unsigned bit_time,
                          unsigned SCL_HIGH, unsigned SDA_HIGH,
                          unsigned S_REST)
{
  timer tmr;
  unsigned fall_time;
  p_i2c <: SDA_HIGH | SCL_HIGH | S_REST;
  delay_ticks(bit_time / 4);
  p_i2c <: SDA_LOW | SCL_HIGH | S_REST;
  delay_ticks(bit_time / 2);
  p_i2c <: SDA_LOW | SCL_LOW | S_REST;
  tmr :> fall_time;
  return fall_time;
}

static void stop_bit(port p_i2c, unsigned bit_time,
                     unsigned SCL_HIGH, unsigned SDA_HIGH,
                     unsigned S_REST, unsigned fall_time)
{
  timer tmr;
  tmr when timerafter(fall_time + bit_time / 4) :> void;
  p_i2c <: SDA_LOW | SCL_LOW | S_REST;
  delay_ticks(bit_time / 4);
  tmr when timerafter(fall_time + bit_time / 2) :> void;
  p_i2c <: SDA_LOW | SCL_HIGH | S_REST;
  #ifdef __XS2A__
    wait_for_clock_high(p_i2c, SCL_HIGH, fall_time, bit_time);
    p_i2c <: SCL_HIGH | SDA_HIGH | S_REST;
  #else
    tmr when timerafter(fall_time + bit_time) :> void;
    p_i2c :> void;
  #endif
  delay_ticks(bit_time/4);
}

static int tx8(port p_i2c, unsigned data, unsigned bit_time,
                unsigned SCL_HIGH, unsigned SDA_HIGH,
                unsigned S_REST, unsigned &fall_time) {
  unsigned CtlAdrsData = ((unsigned) bitrev(data)) >> 24;
  for (int i = 8; i != 0; i--) {
    high_pulse_drive(p_i2c, CtlAdrsData & 1,
                     bit_time, SCL_HIGH, SDA_HIGH, S_REST,
                     fall_time);
    CtlAdrsData >>= 1;
  }
  return high_pulse_sample(p_i2c, bit_time, SCL_HIGH, SDA_HIGH, S_REST,
                           fall_time);
}

[[distributable]]
void i2c_master_single_port(server interface i2c_master_if c[n], unsigned n,
                            port p_i2c,
                            unsigned kbits_per_second,
                            unsigned scl_bit_position,
                            unsigned sda_bit_position,
                            unsigned other_bits_mask) {
  unsigned bit_time = (XS1_TIMER_MHZ * 1000) / kbits_per_second;
  unsigned SDA_HIGH = (1 << sda_bit_position);
  unsigned SCL_HIGH = (1 << scl_bit_position);
  unsigned locked_client = -1;
  #ifdef __XS2A__
  set_port_drive_low(p_i2c);
  p_i2c <: SCL_HIGH | SDA_HIGH | other_bits_mask;
  #else
  p_i2c :> void;
  #endif
  while (1) {
    select {
    case (size_t i =0; i < n; i++)
      (n == 1 || (locked_client == -1 || i == locked_client)) =>
      c[i].read(uint8_t device, uint8_t buf[m], size_t m,
              int send_stop_bit) -> i2c_res_t result:
      #ifndef __XS2A__
      fail("error: single port version of i2c does not support read operations on L/U-series xCORE");
      break;
      #endif
      unsigned fall_time;
      int ack;
      fall_time = start_bit(p_i2c, bit_time, SCL_HIGH, SDA_HIGH,
                            other_bits_mask);
      ack = tx8(p_i2c, (device << 1) | 1, bit_time,
                SCL_HIGH, SDA_HIGH, other_bits_mask, fall_time);
      if (ack == 0) {
        for (int j = 0; j < m; j++){
          unsigned char data = 0;
          timer tmr;
          for (int i = 8; i != 0; i--) {
            int temp = high_pulse_sample(p_i2c, bit_time,
                                         SCL_HIGH, SDA_HIGH, other_bits_mask,
                                         fall_time);
            data = (data << 1) | temp;
          }
          buf[j] = data;

          tmr when timerafter(fall_time + bit_time/4) :> void;
          // ACK after every read byte until the final byte then NACK.
          if (j == m-1)
            p_i2c <: SDA_HIGH | SCL_HIGH | other_bits_mask;
          else {
            p_i2c <: SCL_HIGH | other_bits_mask;
          }
          // High pulse but make sure SDA is not driving before lowering
          // scl
          tmr when timerafter(fall_time + bit_time/2 + bit_time/32) :> void;
          wait_for_clock_high(p_i2c, SCL_HIGH, fall_time, bit_time + 1);
          p_i2c <: SDA_HIGH | other_bits_mask;
          fall_time = fall_time + bit_time;
        }
      }
      if (send_stop_bit) {
        stop_bit(p_i2c, bit_time, SCL_HIGH, SDA_HIGH, other_bits_mask,
                 fall_time);
        locked_client = -1;
      }
      else {
        locked_client = i;
      }

      result = (ack == 0) ? I2C_ACK : I2C_NACK;
      break;
    case (size_t i = 0; i < n; i++)
      (n == 1 || (locked_client == -1 || i == locked_client)) =>
        c[i].write(uint8_t device, uint8_t buf[n], size_t n,
                size_t &num_bytes_sent,
                int send_stop_bit) -> i2c_res_t result:
      unsigned fall_time;
      int ack = 0;
      fall_time = start_bit(p_i2c, bit_time, SCL_HIGH, SDA_HIGH,
                            other_bits_mask);

      ack = tx8(p_i2c, device<<1, bit_time, SCL_HIGH, SDA_HIGH, other_bits_mask,
                fall_time);
      int j = 0;
      for (; j < n; j++) {
        #ifdef __XS2A__
        if (ack != 0)
          break;
        #endif
        ack = tx8(p_i2c, buf[j], bit_time, SCL_HIGH, SDA_HIGH, other_bits_mask,
                  fall_time);
      }
      if (send_stop_bit) {
        stop_bit(p_i2c, bit_time, SCL_HIGH, SDA_HIGH, other_bits_mask,
                 fall_time);
        locked_client = -1;
      } else {
        locked_client = i;
      }
      num_bytes_sent = j;
      #ifdef __XS2A__
        result = (ack == 0) ? I2C_ACK : I2C_NACK;
      #else
        result = I2C_ACK;
      #endif
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
