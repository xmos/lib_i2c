// Copyright (c) 2011, XMOS Ltd, All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>
#ifndef _i2c_h_
#define _i2c_h_

#include <stddef.h>
#include <stdint.h>

/** This type is used in I2C write functions to report back on whether the
 *   write is successful or not.
 */
typedef enum {
  I2C_FAILED,    ///< The write has failed
  I2C_SUCCEEDED, ///< The write was successful
} i2c_res_t;

/** This interface is used to communication with an I2C master component.
 *  It provides facilities for reading and writing to the bus.
 *
 */
typedef interface i2c_master_if {

  /** Write data to an I2C bus.
   *
   *  \param device_addr the address of the slave device to write to
   *  \param buf         the buffer containing data to write
   *  \param n           the number of bytes to write
   *
   *  \returns     whether the write succeeded
   */
  i2c_res_t tx(uint8_t device_addr, uint8_t buf[n], size_t n);

  /** Read data from an I2C bus.
   *
   *  \param device_addr the address of the slave device to read from
   *  \param buf         the buffer to fill with data
   *  \param n           the number of bytes to read
   */
  i2c_res_t rx(uint8_t device_addr, uint8_t buf[n], size_t n);


} i2c_master_if;

extends client interface i2c_master_if : {

  /** Read an 8-bit register on a slave device.
   *
   *  This function reads an 8-bit addressed, 8-bit register from the i2c
   *  bus. The function reads data by
   *  transmitting the register addr and then reading the data from the slave
   *  device.
   *
   *  \param device_addr the address of the slave device to read from
   *  \param reg         the address of the register to read
   *
   *  \returns           the value of the register
   */
  inline uint8_t read_reg(client interface i2c_master_if i,
                          uint8_t device_addr, uint8_t reg) {
    uint8_t a_reg[1] = {reg};
    uint8_t data[1];
    i.tx(device_addr, a_reg, 1);
    i.rx(device_addr, data, 1);
    return data[0];
  }

  /** Write an 8-bit register on a slave device.
   *
   *  This function writes an 8-bit addressed, 8-bit register from the i2c
   *  bus. The function reads data by. The function writes data by
   *  transmitting the register addr and then
   *  transmitting the data to the slave device.
   *
   *  \param device_addr the address of the slave device to write to
   *  \param reg         the address of the register to write
   *  \param data        the 8-bit value to write
   */
  inline void write_reg(client interface i2c_master_if i,
                        uint8_t device_addr, uint8_t reg, uint8_t data) {
    uint8_t a_data[2] = {reg, data};
    i.tx(device_addr, a_data, 2);
  }

  /** Read an 8-bit register on a slave device from a 16 bit register address.
   *
   *  This function reads a 16-bit addressed, 8-bit register from the i2c
   *  bus. The function reads data by
   *  transmitting the register addr and then reading the data from the slave
   *  device.
   *
   *  \param device_addr the address of the slave device to read from
   *  \param reg         the address of the register to read
   *
   *  \returns           the value of the register
   */
  inline uint8_t read_reg8_addr16(client interface i2c_master_if i,
                                  uint8_t device_addr, uint16_t reg) {
    uint8_t a_reg[2] = {reg, reg >> 8};
    uint8_t data[1];
    i.tx(device_addr, a_reg, 2);
    i.rx(device_addr, data, 1);
    return data[0];
  }

  /** Write an 8-bit register on a slave device from a 16 bit register address.
   *
   *  This function writes a 16-bit addressed, 8-bit register from the i2c
   *  bus. The function writes data by
   *  transmitting the register addr and then
   *  transmitting the data to the slave device.
   *
   *  \param device_addr the address of the slave device to write to
   *  \param reg         the address of the register to write
   *  \param data        the 8-bit value to write
   */
  inline void write_reg8_addr16(client interface i2c_master_if i,
                                uint8_t device_addr, uint16_t reg,
                                uint8_t data) {
    uint8_t a_data[3] = {reg, reg >> 8, data};
    i.tx(device_addr, a_data, 3);
  }

  /** Read an 16-bit register on a slave device from a 16 bit register address.
   *
   *  This function reads a 16-bit addressed, 16-bit register from the i2c
   *  bus. The function reads data by
   *  transmitting the register addr and then reading the data from the slave
   *  device.
   *
   *  \param device_addr the address of the slave device to read from
   *  \param reg         the address of the register to read
   *
   *  \returns           the value of the register
   */
  inline uint16_t read_reg16(client interface i2c_master_if i,
                             uint8_t device_addr, uint16_t reg) {
    uint8_t a_reg[2] = {reg, reg >> 8};
    uint8_t data[2];
    i.tx(device_addr, a_reg, 2);
    i.rx(device_addr, data, 2);
    return ((uint16_t) data[0] << 8) | data[1];
  }

  /** Write an 16-bit register on a slave device from a 16 bit register address.
   *
   *  This function writes a 16-bit addressed, 16-bit register from the i2c
   *  bus. The function writes data by
   *  transmitting the register addr and then
   *  transmitting the data to the slave device.
   *
   *  \param device_addr the address of the slave device to write to
   *  \param reg         the address of the register to write
   *  \param data        the 16-bit value to write
   */
  inline void write_reg16(client interface i2c_master_if i,
                          uint8_t device_addr, uint16_t reg,
                          uint16_t data) {
    uint8_t a_data[4] = {reg, reg >> 8, data, data >> 8};
    i.tx(device_addr, a_data, 4);
  }



}


/** Implements I2C on the i2c_master_if interface using two ports.
 *
 *  \param  c      An array of server interface connections for clients to
 *                 connect to
 *  \param  n      The number of clients connected
 *  \param  p_scl  The SCL port of the I2C bus
 *  \param  p_sda  The SDA port of the I2C bus
 *  \param  kbits_per_second The speed of the I2C bus
 **/
[[distributable]] void i2c_master(server interface i2c_master_if c[n],
                                  size_t n,
                                  port p_scl, port p_sda,
                                  unsigned kbits_per_second);

/** Implements I2C on a single multi-bit port.
 *
 *  This function implements an I2C master bus using a single port. However,
 *  There are some restrictions on its use:
 *
 *  - Reading from the bus is not supported.
 *  - Multi master support is not available.
 *
 *  The user needs to be aware that these restrictions are appropriate for the
 *  application.
 *
 *  \param  c      An array of server interface connections for clients to
 *                 connect to
 *  \param  n      The number of clients connected
 *  \param  p_i2c  The multi-bit port containing both SCL and SDA.
 *                 You will need to set the relevant defines in i2c_conf.h in
 *                 you application to say which bits of the port are used
 *  \param  kbits_per_second The speed of the I2C bus
 *  \param  sda_bit_position The bit position of the SDA line on the port
 *  \param  scl_bit_position The bit position of the SCL line on the port
 *  \param  other_bits_mask  The mask for the other bits of the port to use
 *                           when driving it.  Note that, on occassions,
 *                           the other bits are left to float, so external
 *                           resistors shall be used to reinforce the default
 *                           value
 */
[[distributable]]
void i2c_master_single_port(server interface i2c_master_if c[n], size_t n,
                            port p_i2c, unsigned kbits_per_second,
                            unsigned scl_bit_position,
                            unsigned sda_bit_position,
                            unsigned other_bits_mask);



/** This interface is used to communication with an I2C master component
 *  asynchronously.
 *  It provides facilities for reading and writing to the bus.
 *
 */
typedef interface i2c_master_async_if {

  /** Initialize a write to an I2C bus.
   *
   *  \param device_addr the address of the slave device to write to
   *  \param buf         the buffer containing data to write
   *  \param n           the number of bytes to write
   *
   */
  [[guarded]]
  void init_tx(uint8_t device_addr, uint8_t buf[n], size_t n);

  /** Initialize a read to an I2C bus.
   *
   *  \param device_addr the address of the slave device to read from
   *  \param n           the number of bytes to read
   *
   */
  [[guarded]]
  void init_rx(uint8_t device_addr, size_t n);

  /** Completed operation notification.
   *
   *  This notification will fire when a read or write is completed.
   */
  [[notification]]
  slave void completed_operation(void);

  /** Get write result.
   *
   *  This function should be called after a write has completed.
   *
   *  \returns     whether the write succeeded
   */
  [[clears_notification]]
  i2c_res_t get_tx_result(void);


  /** Get read result.
   *
   *  This function should be called after a read has completed.
   *
   *  \param buf         the buffer to fill with data
   *  \param n           the number of bytes to read, this should be the same
   *                     as the number of bytes specified in init_rx(),
   *                     otherwise the behavior is undefined.
   */
  [[clears_notification]]
  void get_rx_data(uint8_t buf[n], size_t n);
} i2c_master_async_if;

/** I2C master component (asynchronous API).
 *
 *  This function implements I2C and allows clients to asynchronously
 *  perform operations on the bus.
 *  Note that this component can be run on the same logical core as other
 *  tasks (i.e. it is [[combinable]]). However, care must be taken that the
 *  other tasks do not take too long in their select cases otherwise this
 *  component may miss I2C transactions.
 *
 *  \param   i            the interface to connect to the client of the
 *                        component
 *  \param  p_scl  The SCL port of the I2C bus
 *  \param  p_sda  The SDA port of the I2C bus
 *  \param  kbits_per_second The speed of the I2C bus
 */
[[combinable]]
void i2c_master_async(client interface i2c_master_async_if i,
                      port p_scl, port p_sda,
                      unsigned kbits_per_second);

/** This interface is used to communication with an I2C slave component.
 *  It provides facilities for reading and writing to the bus. The I2C slave
 *  component acts a *client* to this interface. So the application must
 *  respond to these calls (i.e. the members of the interface are callbacks
 *  to the application).
 *
 */
typedef interface i2c_slave_callback_if {
  /** Master has requested a read.
   *
   *  This function will be called by the component
   *  if the bus master requests a
   *  read from this slave device. The component will clock stretch
   *  until the buffer is filled by the application and then transmit
   *  the buffer back to the master.
   *
   *  \param data   the buffer to fill with data for the master
   *  \param n      the *maximum* number of bytes that can be set. The
   *                exact number of bytes sent back is governed by the bus
   *                master and will depend on the protocol used on the bus.
   */
  void master_requests_read(uint8_t data[n], size_t n);

  /** Master has performed a write.
   *
   *  This function will be called by the component if the bus master
   *  performes a write to this slave device. The component will clock
   *  stretch until the buffer is processed (and then return the ack).
   *
   *  \param data   the buffer filled with data by the master
   *  \param n      the number of bytes transmitted.
   */
  void master_perfomed_write(uint8_t data[n], size_t n);
} i2c_slave_callback_if;


/** I2C slave task.
 *
 *  This function instantiates an i2c_slave component.
 *
 *  \param i   the client end of the i2c_slave_if interface. The component
 *             takes the client end and will make calls on the interface when
 *             the master performs reads or writes.
 *  \param  p_scl  The SCL port of the I2C bus
 *  \param  p_sda  The SDA port of the I2C bus
 *  \param device_addr The address of the slave device
 *  \param max_transaction_size  The maximum number of bytes that will be
 *                               read or written by the master.
 *
 */
[[combinable]]
void i2c_slave(client i2c_slave_callback_if i,
               port p_scl, port p_sda,
               uint8_t device_addr,
               static const size_t max_transaction_size);



#endif
