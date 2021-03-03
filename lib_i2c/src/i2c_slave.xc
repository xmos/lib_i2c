// Copyright (c) 2014-2021, XMOS Ltd, All rights reserved
// This software is available under the terms provided in LICENSE.txt.
#include <i2c.h>
#include <xs1.h>
#include <xclib.h>

enum i2c_slave_state {
  WAITING_FOR_START_OR_STOP,
  READING_ADDR,
  ACK_ADDR,
  ACK_WAIT_HIGH,
  ACK_WAIT_LOW,
  IGNORE_ACK,
  MASTER_WRITE,
  MASTER_READ
};

static inline void ensure_setup_time()
{
  // The I2C spec requires a 100ns setup time
  delay_ticks(10);
}

[[combinable]]
void i2c_slave(client i2c_slave_callback_if i,
               port p_scl, port p_sda,
               uint8_t device_addr)
{
  enum i2c_slave_state state = WAITING_FOR_START_OR_STOP;
  enum i2c_slave_state next_state = WAITING_FOR_START_OR_STOP;
  int sda_val = 0;
  int scl_val;
  int bitnum = 0;
  int data;
  int rw = 0;
  int stop_bit_check = 0;
  int ignore_stop_bit = 1;
  p_sda when pinseq(1) :> void;
  while (1) {
    select {
    case i.shutdown():
      return;
    case state != WAITING_FOR_START_OR_STOP => p_scl when pinseq(scl_val) :> void:
      switch (state) {
      case READING_ADDR:
        // If clock has gone low, wait for it to go high before doing anything
        if (scl_val == 0) {
          scl_val = 1;
          break;
        }

        int bit;
        p_sda :> bit;
        if (bitnum < 7) {
          data = (data << 1) | bit;
          bitnum++;
          scl_val = 0;
          break;
        }

        // We have gathered the whole device address sent by the master
        if (data != device_addr) {
          state = IGNORE_ACK;
        } else {
          state = ACK_ADDR;
          rw = bit;
        }
        scl_val = 0;
        break;

      case IGNORE_ACK:
        // This request is not for us, ignore the ACK
        next_state = WAITING_FOR_START_OR_STOP;
        scl_val = 1;
        state = ACK_WAIT_HIGH;
        break;

      case ACK_ADDR:
        // Stretch clock (hold low) while application code is called
        p_scl <: 0;

        // Callback to the application to determine whether to ACK
        // or NACK the address.
        int ack;
        if (rw) {
          ack = i.ack_read_request();
        } else {
          ack = i.ack_write_request();
        }

        ignore_stop_bit = 0;
        if (ack == I2C_SLAVE_NACK) {
          // Release the data line so that it is pulled high
          p_sda :> void;
          next_state = WAITING_FOR_START_OR_STOP;
        } else {
          // Drive the ACK low
          p_sda <: 0;
          if (rw) {
            next_state = MASTER_READ;
          } else {
            next_state = MASTER_WRITE;
          }
        }
        scl_val = 1;
        state = ACK_WAIT_HIGH;

        ensure_setup_time();

        // Release the clock
        p_scl :> void;
        break;

      case ACK_WAIT_HIGH:
        // Rising edge of clock, hold ack to the falling edge
        state = ACK_WAIT_LOW;
        scl_val = 0;
        break;

      case ACK_WAIT_LOW:
        // ACK done, release the data line
        p_sda :> void;
        if (next_state == MASTER_READ) {
          scl_val = 0;
        } else if (next_state == MASTER_WRITE) {
          data = 0;
          scl_val = 1;
        } else { // WAITING_FOR_START_OR_STOP
          sda_val = 0;
        }
        state = next_state;
        bitnum = 0;
        break;

      case MASTER_READ:
        if (scl_val == 1) {
          // Rising edge
          if (bitnum == 8) {
            // Sample ack from master
            int bit;
            p_sda :> bit;
            if (bit) {
              // Master has NACKed so the transaction is finished
              state = WAITING_FOR_START_OR_STOP;
              sda_val = 0;
            } else {
              bitnum = 0;
              scl_val = 0;
            }
          } else {
            // Wait for next falling edge
            scl_val = 0;
            bitnum++;
          }
        } else {
          // Falling edge, drive data
          if (bitnum < 8) {
            if (bitnum == 0) {
              // Stretch clock (hold low) while application code is called
              p_scl <: 0;
              data = i.master_requires_data();
              // Data is transmitted MSB first
              data = bitrev(data) >> 24;

              // Send first bit of data
              p_sda <: data & 0x1;

              ensure_setup_time();

              // Release the clock
              p_scl :> void;
            } else {
              p_sda <: data & 0x1;
            }
            data >>= 1;
          } else {
            // Release the bus for the master to be able to ACK/NACK
            p_sda :> void;
          }
          scl_val = 1;
        }
        break;

      case MASTER_WRITE:
        if (scl_val == 1) {
          // Rising edge
          int bit;
          p_sda :> bit;
          data = (data << 1) | (bit & 0x1);
          if (bitnum == 0) {
            if (bit) {
              sda_val = 0;
            } else {
              sda_val = 1;
            }
            // First bit could be a start or stop bit
            stop_bit_check = 1;
          }
          scl_val = 0;
          bitnum++;
        } else {
          // Falling edge

          // Not a start or stop bit
          stop_bit_check = 0;

          if (bitnum == 8) {
            // Stretch clock (hold low) while application code is called
            p_scl <: 0;
            int ack = i.master_sent_data(data);
            if (ack == I2C_SLAVE_NACK) {
              // Release the data bus so it is pulled high to signal NACK
              p_sda :> void;
            } else {
              // Drive data bus low to signal ACK
              p_sda <: 0;
            }
            state = ACK_WAIT_HIGH;

            ensure_setup_time();

            // Release the clock
            p_scl :> void;
          }
          scl_val = 1;
        }
        break;
      }
      break;

    case (state == WAITING_FOR_START_OR_STOP) || stop_bit_check =>
            p_sda when pinseq(sda_val) :> void:
      if (sda_val == 1) {
        // SDA has transitioned from low to high, if SCL is high
        // then it is a stop bit.
        int val;
        p_scl :> val;
        if (val) {
          if (!ignore_stop_bit) {
            i.stop_bit();
          }
          state = WAITING_FOR_START_OR_STOP;
          ignore_stop_bit = 1;
          stop_bit_check = 0;
        }
        sda_val = 0;
      } else {
        // SDA has transitioned from high to low, if SCL is high
        // then it is a start bit.
        int val;
        p_scl :> val;
        if (val == 1) {
          state = READING_ADDR;
          bitnum = 0;
          data = 0;
          scl_val = 0;
          stop_bit_check = 0;
        } else {
          sda_val = 1;
        }
      }
      break;
    }
  }
}
