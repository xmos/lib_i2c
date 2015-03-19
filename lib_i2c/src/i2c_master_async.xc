// Copyright (c) 2015, XMOS Ltd, All rights reserved
#include <i2c.h>
#include <xs1.h>
#include <string.h>
#include <print.h>
#include <syscall.h>
#include <xassert.h>

enum i2c_async_master_state_t {
  IDLE,
  START_BIT_0,
  START_BIT_1,
  WRITE_0,
  WRITE_1,
  WRITE_ACK_0,
  WRITE_ACK_1,
  WRITE_ACK_2,
  READ_0,
  READ_1,
  READ_2,
  READ_ACK_0,
  READ_ACK_1,
  STOP_BIT_0,
  STOP_BIT_1,
  STOP_BIT_2,
  STOP_BIT_3,
  STOP_BIT_4,
};

enum optype_t {
  WRITE = 0, READ = 1, SEND_STOP_BIT = 2
};

enum ack_t {
  ACKED = 0, NACKED = 1,
};


void i2c_master_async_aux(server interface i2c_master_async_if i[n],
                          size_t n,
                          client interface i2c_master_if i2c,
                          static const size_t max_transaction_size)
{
  uint8_t buf[max_transaction_size];
  uint8_t device_addr;
  size_t num_bytes, num_bytes_sent;
  int send_stop_bit;
  int optype;
  int cur_client;
  i2c_res_t res;

  while (1) {
    select {
    case i[int j].write(uint8_t addr, uint8_t buf0[n], size_t n,
                        int ssb):
      device_addr = addr;
      send_stop_bit = ssb;
      optype = WRITE;
      num_bytes = n;
      memcpy(buf, buf0, n);
      cur_client = j;
      break;
    case i[int j].read(uint8_t addr, size_t n, int ssb):
      device_addr = addr;
      send_stop_bit = ssb;
      optype = READ;
      num_bytes = n;
      cur_client = j;
      break;
    case i[int j].send_stop_bit():
      optype = WRITE;
      cur_client = j;
      break;
    case i[int j].shutdown():
      return;
    case i[int j].get_write_result(size_t &nbs) -> i2c_res_t result:
      // ERROR
      break;
    case i[int j].get_read_data(uint8_t buf0[n], size_t n) -> i2c_res_t result:
      // ERROR
      break;

    }
    switch (optype) {
    case WRITE:
      res = i2c.write(device_addr, buf, num_bytes, num_bytes_sent,
                      send_stop_bit);
      break;
    case READ:
      res = i2c.read(device_addr, buf, num_bytes, send_stop_bit);
      break;
    case SEND_STOP_BIT:
      i2c.send_stop_bit();
      break;
    }

    i[cur_client].operation_complete();

    select {
    case i[int j].get_write_result(size_t &nbs) -> i2c_res_t result:
      nbs = num_bytes_sent;
      result = res;
      break;
    case i[int j].get_read_data(uint8_t buf0[n], size_t n) -> i2c_res_t result:
      memcpy(buf0, buf, n);
      result = res;
      break;
    case i[int j].shutdown():
      return;
    case i[int j].send_stop_bit():
      break;
    }
  }
}


void i2c_master_async(server interface i2c_master_async_if i[n],
                      size_t n,
                      port p_scl, port p_sda,
                      unsigned kbits_per_second,
                      static const size_t max_transaction_size)
{
  i2c_master_if i2c_dist[1];
  par {
    i2c_master(i2c_dist, 1, p_scl, p_sda, kbits_per_second);
    i2c_master_async_aux(i, n, i2c_dist[0], max_transaction_size);
  }
}


/*  Adjust for time slip.
 *
 *  All timings of the state machine in i2c_master_async_comb are made
 *  w.r.t the measured fall_time of the SCL line.
 *  If the task keeps up then we will get the speed requested. However,
 *  if the task falls behind due to other processing on the core then
 *  this function will adjust both the next event time and the fall time
 *  reference to slip so that subsequent timings are correct.
 */
static void inline adjust_for_slip(int now,
                                   int &event_time,
                                   int &?fall_time)
{
  // This value is the minimum number of timer ticks we estimate we can
  // get to a new event to from now.
  const int SLIP_THRESHOLD = 100;
  if (event_time - now < SLIP_THRESHOLD) {
    int new_event_time = now + SLIP_THRESHOLD;
    if (!isnull(fall_time)) {
      fall_time += new_event_time  - event_time;
    }
    event_time = new_event_time;
  }
}

static int inline adjust_fall(int event_time, int now, int fall_time)
{
  const int SLIP_THRESHOLD = 100;
  if (now - event_time > SLIP_THRESHOLD) {
    fall_time = fall_time + (now - event_time);
  }
  return fall_time;
}
  

[[combinable]]
void i2c_master_async_comb(server interface i2c_master_async_if i[n],
                           size_t n,
                           port p_scl, port p_sda,
                           unsigned kbits_per_second,
                           static const size_t max_transaction_size)
{

  uint8_t buf[max_transaction_size];
  timer tmr;
  unsigned bit_time = (XS1_TIMER_MHZ * 1000) / kbits_per_second;
  int state = IDLE;
  int waiting_for_clock_release = 0, timer_enabled = 0;
  int optype;
  int fall_time;
  int data;
  int bitnum;
  int bytes_sent;
  int num_bytes;
  int event_time;
  int cur_client;
  i2c_res_t res;
  /* These select cases represent the main state machine for the I2C master
     component. The state machine will change state based on a timer event to
     progress the transaction or on an event from the SCL line when waiting
     for the clock to be released (supporting clock stretching). */
  while (1) {
    select {
    case waiting_for_clock_release => p_scl when pinseq(1) :> void:
      int now;
      tmr :> now;
      switch (state) {
      case WRITE_0:
      case WRITE_ACK_0:
      case READ_ACK_0:
      case STOP_BIT_0:
      case READ_0:
        fall_time = fall_time + bit_time;
        event_time = fall_time;
        adjust_for_slip(now, event_time, fall_time);
        break;
      case WRITE_ACK_1:
        fall_time = fall_time + bit_time;
        event_time = fall_time - bit_time / 4;
        adjust_for_slip(now, event_time, fall_time);
        state = WRITE_ACK_2;
        break;
      case STOP_BIT_2:
        event_time = now + bit_time / 2;
        adjust_for_slip(now, event_time, null);
        state = STOP_BIT_3;
        break;
      case READ_1:
        fall_time = fall_time + bit_time;
        event_time = fall_time - bit_time / 4;
        adjust_for_slip(now, event_time, fall_time);
        state = READ_2;
        break;
      }
      waiting_for_clock_release = 0;
      timer_enabled = 1;
      break;
    case timer_enabled => tmr when timerafter(event_time) :> int now:
      switch (state) {
      case START_BIT_0:
        p_sda <: 0;
        event_time = now + bit_time / 2;
        state = START_BIT_1;
        break;
      #pragma fallthrough
      case START_BIT_1:
        fall_time = now;
        // Fallthrough to WRITE_0 state
      case WRITE_0:
        p_scl <: 0;
        fall_time = adjust_fall(event_time, now, fall_time);
        p_sda <: data >> 7;
        data <<= 1;
        bitnum++;
        state = WRITE_1;
        event_time = fall_time + bit_time / 2 + bit_time / 32;
        adjust_for_slip(now, event_time, fall_time);
        break;
      case WRITE_1:
        p_scl :> void;
        timer_enabled = 0;
        waiting_for_clock_release = 1;
        if (bitnum == 8)  {
          state = WRITE_ACK_0;
        }
        else {
          state = WRITE_0;
        }
        break;
      case WRITE_ACK_0:
        p_scl <: 0;
        p_sda :> void;
        fall_time = adjust_fall(event_time, now, fall_time);
        event_time = fall_time + bit_time / 2 + bit_time / 32;
        adjust_for_slip(now, event_time, fall_time);
        state = WRITE_ACK_1;
        break;
      case WRITE_ACK_1:
        p_scl :> void;
        timer_enabled = 0;
        waiting_for_clock_release = 1;
        break;
      case WRITE_ACK_2:
        int ack;
        p_sda :> ack;
        event_time = fall_time;
        adjust_for_slip(now, event_time, fall_time);
        if (ack == ACKED && optype == WRITE) {
          bytes_sent++;
          int all_data_sent = (bytes_sent == num_bytes);
          if (all_data_sent) {
            // The master and slave disagree since the slave should nack
            // the last byte.
            res = I2C_ACK;
            state = STOP_BIT_0;
          } else {
            // get next byte of data.
            data = buf[bytes_sent];
            bitnum = 0;
            // Now go back to the transmitting
            state = WRITE_0;
          }
        } else if (ack == NACKED && optype == WRITE) {
          bytes_sent++;
          int all_data_sent = (bytes_sent == num_bytes);
          if (all_data_sent) {
            // The master and slave agree that this is the end of the operation.
            res = I2C_NACK;
            state = STOP_BIT_0;
          } else {
            // The slave has aborted the operation.
            res = I2C_NACK;
            state = STOP_BIT_0;
          }
        } else if (ack == ACKED  && optype == READ) {
          // The slave has acked the addr, we can go ahead with the operation.
          data = 0;
          bitnum = 0;
          bytes_sent++;
          state = READ_0;
          res = I2C_ACK;
        } else if (ack == NACKED && optype == READ) {
          // The slave has nacked the addr (or the slave isn't there). Abort.
          res = I2C_NACK;
          state = STOP_BIT_0;
        }
        break;
      case READ_0:
        p_scl <: 0;
        p_sda :> void;
        fall_time = adjust_fall(event_time, now, fall_time);
        bitnum++;
        state = READ_1;
        event_time = fall_time + bit_time / 2 + bit_time / 32;
        adjust_for_slip(now, event_time, fall_time);
        break;
      case READ_1:
        p_scl :> void;
        timer_enabled = 0;
        waiting_for_clock_release = 1;
        break;
      case READ_2:
        int bit;
        p_sda :> bit;
        data <<= 1;
        data += bit&1;
        event_time = fall_time;
        adjust_for_slip(now, event_time, fall_time);
        if (bitnum == 8)  {
          buf[bytes_sent] = data;
          bytes_sent++;
          state = READ_ACK_0;
        }
        else {
          state = READ_0;
        }
        break;
      case READ_ACK_0:
        p_scl <: 0;
        if (bytes_sent == num_bytes)
          p_sda :> void;
        else
          p_sda <: 0;
        fall_time = adjust_fall(event_time, now, fall_time);
        state = READ_ACK_1;
        event_time = fall_time + bit_time / 2 + bit_time / 32;
        adjust_for_slip(now, event_time, fall_time);
        break;
      case READ_ACK_1:
        p_scl :> void;
        timer_enabled = 0;
        waiting_for_clock_release = 1;
        if (bytes_sent == num_bytes)
          state = STOP_BIT_0;
        else {
          data = 0;
          bitnum = 0;
          state = READ_0;
        }
        break;
      case STOP_BIT_0:
        p_scl <: 0;
        fall_time = adjust_fall(event_time, now, fall_time);
        event_time = fall_time + bit_time / 4;
        adjust_for_slip(now, event_time, fall_time);
        state = STOP_BIT_1;
        break;
      case STOP_BIT_1:
        p_sda <: 0;
        event_time = fall_time + bit_time / 2;
        adjust_for_slip(now, event_time, fall_time);
        state = STOP_BIT_2;
        break;
      case STOP_BIT_2:
        p_scl :> void;
        timer_enabled = 0;
        waiting_for_clock_release = 1;
        break;
      case STOP_BIT_3:
        p_sda :> void;
        event_time = now + bit_time/4;
        state = STOP_BIT_4;
        break;
      case STOP_BIT_4:
        i[cur_client].operation_complete();
        timer_enabled = 0;
        state = IDLE;
        break;
      case IDLE:
        fail();
        break;
      }
      break;
    case i[int j].write(uint8_t device_addr, uint8_t buf0[n], size_t n,
                        int send_stop_bit):
      data = (device_addr << 1) | 0;
      bitnum = 0;
      optype = WRITE;
      num_bytes = n;
      memcpy(buf, buf0, n);
      // The 'bytes_sent' variable gets increment after every byte *including*
      // the addr bytes. So we set it to -1 to be 0 after the addr byte.
      bytes_sent = -1;
      timer_enabled = 1;
      cur_client = j;
      tmr :> event_time;
      state = START_BIT_0;
      break;

    case i[int j].read(uint8_t device_addr, size_t n, int send_stop_bit):
      data = (device_addr << 1) | 1;
      bitnum = 0;
      optype = READ;
      num_bytes = n;
      // The 'bytes_sent' variable gets increment after every byte *including*
      // the addr bytes. So we set it to -1 to be 0 after the addr byte.
      bytes_sent = -1;
      timer_enabled = 1;
      cur_client = j;
      tmr :> event_time;
      state = START_BIT_0;
      break;

    case i[int j].send_stop_bit():
      break;

    case i[int j].get_write_result(size_t &num_bytes_sent) -> i2c_res_t result:
      num_bytes_sent = bytes_sent;
      result = res;
      break;

    case i[int j].get_read_data(uint8_t buf0[n], size_t n) -> i2c_res_t result:
      memcpy(buf0, buf, n);
      result = res;
      break;

    case i[int j].shutdown():
      return;
    }
  }
}
