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

static inline void wait_quarter(unsigned bit_time) {
    delay_ticks((bit_time + 3)/4);
}

static inline void wait_half(unsigned bit_time) {
    wait_quarter(bit_time);
    wait_quarter(bit_time);
}

static void high_pulse_drive(port p_i2c, int sdaValue, unsigned bit_time,
                             unsigned SCL_HIGH, unsigned SDA_HIGH,
                             unsigned S_REST,
                             unsigned &fall_time)
{
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
}

static int high_pulse_sample(port p_i2c, int expectedSDA, unsigned bit_time,
                             unsigned SCL_HIGH, unsigned SDA_HIGH,
                             unsigned S_REST,
                             unsigned &fall_time)
{
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
}

static unsigned start_bit(port p_i2c, unsigned bit_time,
                          unsigned SCL_HIGH, unsigned SDA_HIGH,
                          unsigned S_REST)
{
  timer tmr;
  unsigned fall_time;
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
  wait_quarter(bit_time);
  tmr when timerafter(fall_time + bit_time / 2) :> void;
  p_i2c <: SDA_LOW | SCL_HIGH | S_REST;
  tmr when timerafter(fall_time + bit_time) :> void;
  p_i2c :> void;
  delay_ticks(bit_time/4);
}

static void tx8(port p_i2c, unsigned data, unsigned bit_time,
                unsigned SCL_HIGH, unsigned SDA_HIGH,
                unsigned S_REST, unsigned &fall_time) {
    unsigned CtlAdrsData = ((unsigned) bitrev(data)) >> 24;
    for (int i = 8; i != 0; i--) {
      high_pulse_drive(p_i2c, CtlAdrsData & 1,
                       bit_time, SCL_HIGH, SDA_HIGH, S_REST,
                       fall_time);
      CtlAdrsData >>= 1;
    }
    high_pulse_sample(p_i2c, 0, bit_time, SCL_HIGH, SDA_HIGH, S_REST,
                      fall_time);
    return;
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
  p_i2c :> void;    // Drive all high
  while (1) {
    select {
    case c[int i].read(uint8_t device, uint8_t buf[n], size_t n,
                     int send_stop_bit) -> i2c_res_t result:
      fail("error: single port version of i2c does not support read operations");
      break;
    case c[int i].write(uint8_t device, uint8_t buf[n], size_t n,
                     size_t &num_bytes_sent,
                     int send_stop_bit)  -> i2c_res_t result:
      unsigned fall_time;
      fall_time = start_bit(p_i2c, bit_time, SCL_HIGH, SDA_HIGH,
                            other_bits_mask);

      tx8(p_i2c, device<<1, bit_time, SCL_HIGH, SDA_HIGH, other_bits_mask,
          fall_time);
      for (int j = 0; j < n; j++) {
        tx8(p_i2c, buf[j], bit_time, SCL_HIGH, SDA_HIGH, other_bits_mask,
            fall_time);
      }
      if (send_stop_bit)
        stop_bit(p_i2c, bit_time, SCL_HIGH, SDA_HIGH, other_bits_mask,
                 fall_time);
      num_bytes_sent = n;
      result = I2C_ACK;
      break;
    case c[int i].send_stop_bit(void):
      timer tmr;
      unsigned fall_time;
      tmr :> fall_time;
      stop_bit(p_i2c, bit_time, SCL_HIGH, SDA_HIGH, other_bits_mask, fall_time);
      break;
    case c[int i].shutdown():
      return;
    }
  }
}
