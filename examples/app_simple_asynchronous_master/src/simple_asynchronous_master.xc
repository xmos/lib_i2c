// Copyright (c) 2018-2021, XMOS Ltd, All rights reserved
// This software is available under the terms provided in LICENSE.txt.

/* A simple application example used for code snippets in the library
 * documentation.
 */

#include <xs1.h>
#include <stdio.h>
#include "i2c.h"

void my_application(client i2c_master_async_if i2c, uint8_t target_device_addr);
void my_application_handle_bus_error(i2c_res_t result);
void my_application_fill_buffer(uint8_t buffer[]);

// I2C interface ports
port p_scl = XS1_PORT_1E;
port p_sda = XS1_PORT_1F;

#define BUFFER_BYTES 100

int main(void) {
  i2c_master_async_if i2c[1];
  static const uint8_t target_device_addr = 0x3c;

  par {
    i2c_master_async(i2c, 1, p_scl, p_sda, 100, BUFFER_BYTES);
    my_application(i2c[0], target_device_addr);
  }
  return 0;
}

void my_application(client i2c_master_async_if i2c, uint8_t target_device_addr) {
  uint8_t buffer[BUFFER_BYTES];

  // Create and send initial block of data
  my_application_fill_buffer(buffer);
  i2c.write(target_device_addr, buffer, BUFFER_BYTES, 1);

  // Start computing the next block of data
  my_application_fill_buffer(buffer);

  while (1) {
    select {
      case i2c.operation_complete():
        size_t num_bytes_sent;
        i2c_res_t result = i2c.get_write_result(num_bytes_sent);
        if (num_bytes_sent != BUFFER_BYTES) {
           my_application_handle_bus_error(result);
        }

        // Offload the next data bytes to be sent
        i2c.write(target_device_addr, buffer, BUFFER_BYTES, 1);

        // Compute the next block of data
        my_application_fill_buffer(buffer);

        break;
    }
  }
}

void my_application_handle_bus_error(i2c_res_t result) {
  // Ignore for now
}

void my_application_fill_buffer(uint8_t buffer[]) {
  static int offset = 0;
  for (int i = 0; i < BUFFER_BYTES; ++i) {
    buffer[i] = i + offset;
  }
  offset += 1;
}

// end
