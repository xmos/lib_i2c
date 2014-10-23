#include <i2c.h>
#include <xs1.h>
#include <xclib.h>
#include <timer.h>

static void release_clock_and_wait(port i2c_scl,
                                   unsigned &fall_time,
                                   unsigned delay)
{
  timer tmr;
  unsigned time;
  i2c_scl when pinseq(1) :> void;
  tmr when timerafter(fall_time + delay) :> time;
  // Adjust timing due to clock stretching
  fall_time = time - delay;
}

static int high_pulse_sample(port i2c_scl, port ?i2c_sda,
                             unsigned bit_time,
                             unsigned &fall_time) {
  int sample_value;
  timer tmr;
  if (!isnull(i2c_sda)) {
    i2c_sda :> int _;
  }
  tmr when timerafter(fall_time + bit_time / 2 + bit_time / 32) :> void;
  release_clock_and_wait(i2c_scl, fall_time, (bit_time * 3) / 4);
  if (!isnull(i2c_sda)) {
    i2c_sda :> sample_value;
  }
  fall_time = fall_time + bit_time;
  tmr when timerafter(fall_time) :> void;
  i2c_scl <: 0;
  return sample_value;
}

static void high_pulse(port i2c_scl, unsigned bit_time,
                       unsigned &fall_time)
{
  high_pulse_sample(i2c_scl, null, bit_time, fall_time);
}

static unsigned start_bit(port i2c_scl, port i2c_sda,
                          unsigned bit_time) {
  timer tmr;
  unsigned fall_time;
  delay_ticks(bit_time / 4);
  i2c_sda  <: 0;
  delay_ticks(bit_time / 2);
  i2c_scl  <: 0;
  tmr :> fall_time;
  return fall_time;
}

static void stop_bit(port i2c_scl, port i2c_sda, unsigned bit_time,
                     unsigned fall_time) {
  timer tmr;
  tmr when timerafter(fall_time + bit_time / 4) :> void;
  i2c_sda <: 0;
  tmr when timerafter(fall_time + bit_time / 2) :> void;
  release_clock_and_wait(i2c_scl, fall_time, bit_time);
  i2c_sda :> void;
  delay_ticks(bit_time/4);
}

static int tx8(port p_scl, port p_sda, unsigned data,
               unsigned bit_time,
               unsigned &fall_time) {
  unsigned CtlAdrsData = ((unsigned) bitrev(data)) >> 24;
  for (int i = 8; i != 0; i--) {
    p_sda <: >> CtlAdrsData;
    high_pulse(p_scl, bit_time, fall_time);
  }
  return high_pulse_sample(p_scl, p_sda, bit_time, fall_time);
}


[[distributable]]
void i2c_master(server interface i2c_master_if c[n], size_t n,
                port p_scl, port p_sda, unsigned kbits_per_second,
                i2c_enable_mm_t enable_multimaster)
{
  unsigned bit_time = (XS1_TIMER_MHZ * 1000) / kbits_per_second;
  p_scl :> void;
  p_sda :> void;
  while (1) {
    select {
    case c[int i].rx(uint8_t device, uint8_t buf[m], size_t m):
      unsigned fall_time;
      int ack;
      fall_time = start_bit(p_scl, p_sda, bit_time);
      ack = tx8(p_scl, p_sda, (device << 1) | 1, bit_time, fall_time);

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
          // High pulse but make sure SDA is not driving before lowering
          // scl
          tmr when timerafter(fall_time + bit_time/2 + bit_time/32) :> void;
          release_clock_and_wait(p_scl, fall_time, bit_time);
          p_sda :> void;
          p_scl <: 0;
          fall_time = fall_time + bit_time;
        }
      }
      stop_bit(p_scl, p_sda, bit_time, fall_time);

      break;

    case c[int i].tx(uint8_t device, uint8_t buf[n], size_t n) -> i2c_write_res_t result:
      int ack = 0;
      unsigned fall_time;
      fall_time = start_bit(p_scl, p_sda, bit_time);
      ack = tx8(p_scl, p_sda, (device << 1), bit_time, fall_time);
      for (int j = 0; j < n; j++) {
        if (ack == 0) {
          ack |= tx8(p_scl, p_sda, buf[j], bit_time, fall_time);
        }
      }
      stop_bit(p_scl, p_sda, bit_time, fall_time);
      result = (ack == 0) ? I2C_WRITE_ACK_SUCCEEDED : I2C_WRITE_ACK_FAILED;
      break;
    }
  }
}

