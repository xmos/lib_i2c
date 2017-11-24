// Copyright (c) 2011-2016, XMOS Ltd, All rights reserved
#include <i2c.h>
#include <xs1.h>
#include <xclib.h>
#include <timer.h>

static const unsigned inline compute_low_period_time(unsigned bit_time)
{
  // Ensure the clock low period is longer than the high period
  return bit_time/2 + bit_time/16;
}

static const unsigned inline compute_bus_off_time(unsigned bit_time)
{
  return bit_time/2 + bit_time/16;
}

/** Releases the SCL line, reads it back and waits until it goes high (in
 *  case the slave is clock stretching).
 *  Since the line going high may be delayed, the fall_time value may
 *  need to be adjusted
 */
static void release_clock_and_wait(port i2c_scl,
                                   unsigned &fall_time,
                                   unsigned delay)
{
  i2c_scl when pinseq(1) :> void;

  timer tmr;
  unsigned time;
  tmr when timerafter(fall_time + delay) :> time;

  // Adjust timing due to clock stretching without clock drift in the normal case
  if (time > fall_time + delay + 10) {
    fall_time = time - delay;
  }
}

/** 'Pulse' the clock line high and in the middle of the high period
 *  sample the data line (if required). Timing is done via the fall_time
 *  reference and bit_time period supplied.
 */
static int high_pulse_sample(port i2c_scl, port i2c_sda,
                             unsigned bit_time,
                             unsigned &fall_time)
{
  int sample_value = 0;
  timer tmr;
  i2c_sda :> int _;
  tmr when timerafter(fall_time + compute_low_period_time(bit_time)) :> void;
  release_clock_and_wait(i2c_scl, fall_time, (bit_time * 3) / 4);
  i2c_sda :> sample_value;
  fall_time = fall_time + bit_time;
  tmr when timerafter(fall_time) :> void;
  i2c_scl <: 0;
  return sample_value;
}

/** 'Pulse' the clock line high. Timing is done via the fall_time
 *  reference and bit_time period supplied.
 */
static void high_pulse(port i2c_scl, unsigned bit_time,
                       unsigned &fall_time)
{
  timer tmr;
  tmr when timerafter(fall_time + compute_low_period_time(bit_time)) :> void;
  release_clock_and_wait(i2c_scl, fall_time, (bit_time * 3) / 4);
  fall_time = fall_time + bit_time;
  tmr when timerafter(fall_time) :> void;
  i2c_scl <: 0;
}

/** Output a start bit. The function returns the 'fall time' i.e. the
 *  reference clock time when the SCL line transitions to low.
 */
static void start_bit(port i2c_scl, port i2c_sda,
                      unsigned bit_time,
                      unsigned &fall_time,
                      int stopped)
{
  timer tmr;

  if (!stopped) {
    tmr when timerafter(fall_time + compute_low_period_time(bit_time)) :> void;
    // Release the SCL to allow it to be pulled high
    i2c_scl :> void;
    delay_ticks(compute_bus_off_time(bit_time));
  }

  // Drive SDA low
  i2c_sda  <: 0;
  delay_ticks(bit_time / 2);
  // Drive SCL low
  i2c_scl  <: 0;

  // Record
  tmr :> fall_time;
}

/** Output a stop bit.
 */
static void stop_bit(port i2c_scl, port i2c_sda, unsigned bit_time,
                     unsigned fall_time)
{
  timer tmr;
  i2c_sda <: 0;
  tmr when timerafter(fall_time + compute_low_period_time(bit_time)) :> void;
  release_clock_and_wait(i2c_scl, fall_time, bit_time);
  i2c_sda :> void;
  delay_ticks(compute_bus_off_time(bit_time));
}

/** Transmit 8 bits of data, then read the ack back from the slave and return
 *  that value.
 */
static int tx8(port p_scl, port p_sda, unsigned data,
               unsigned bit_time, unsigned &fall_time)
{
  // Data is transmitted MSB first
  data = bitrev(data) >> 24;
  for (int i = 8; i != 0; i--) {
    p_sda <: data & 0x1;
    data >>= 1;
    high_pulse(p_scl, bit_time, fall_time);
  }
  return high_pulse_sample(p_scl, p_sda, bit_time, fall_time);
}

[[distributable]]
void i2c_master(server interface i2c_master_if c[n], size_t n,
                port p_scl, port p_sda, unsigned kbits_per_second)
{
  unsigned last_fall_time = 0;
  unsigned bit_time = (XS1_TIMER_MHZ * 1000) / kbits_per_second;
  unsigned locked_client = -1;
  p_scl :> void;
  p_sda :> void;
  while (1) {
    select {

    case (size_t i =0; i < n; i++)
      (n == 1 || (locked_client == -1 || i == locked_client)) =>
      c[i].read(uint8_t device, uint8_t buf[m], size_t m,
              int send_stop_bit) -> i2c_res_t result:

      const int stopped = locked_client == -1;
      unsigned fall_time = last_fall_time;
      start_bit(p_scl, p_sda, bit_time, fall_time, stopped);
      int ack = tx8(p_scl, p_sda, (device << 1) | 1, bit_time, fall_time);

      if (ack == 0) {
        for (int j = 0; j < m; j++){
          unsigned char data = 0;
          timer tmr;
          for (int i = 8; i != 0; i--) {
            int temp = high_pulse_sample(p_scl, p_sda, bit_time, fall_time);
            data = (data << 1) | temp;
          }
          buf[j] = data;

          tmr when timerafter(fall_time + bit_time/4) :> void;
          // ACK after every read byte until the final byte then NACK.
          if (j == m-1)
            p_sda :> void;
          else {
            p_sda <: 0;
          }
          // High pulse but make sure SDA is not driving before lowering SCL
          tmr when timerafter(fall_time + compute_low_period_time(bit_time)) :> void;
          release_clock_and_wait(p_scl, fall_time, bit_time);
          p_scl <: 0;
          p_sda :> void;
          fall_time = fall_time + bit_time;
        }
      }
      if (send_stop_bit) {
        stop_bit(p_scl, p_sda, bit_time, fall_time);
        locked_client = -1;
      }
      else {
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
      unsigned fall_time = last_fall_time;
      const int stopped = locked_client == -1;
      start_bit(p_scl, p_sda, bit_time, fall_time, stopped);
      int ack = tx8(p_scl, p_sda, (device << 1), bit_time, fall_time);
      int j = 0;
      for (; j < n; j++) {
        if (ack != 0) {
          break;
        }
        ack = tx8(p_scl, p_sda, buf[j], bit_time, fall_time);
      }
      if (send_stop_bit) {
        stop_bit(p_scl, p_sda, bit_time, fall_time);
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
      stop_bit(p_scl, p_sda, bit_time, fall_time);
      locked_client = -1;
      break;

    case c[int i].shutdown(void):
      return;
    }
  }
}

