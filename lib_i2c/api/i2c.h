// Copyright (c) 2015, XMOS Ltd, All rights reserved
#ifndef _i2c_h_
#define _i2c_h_

#include <stddef.h>
#include <stdint.h>

/** This type is used in I2C functions to report back on whether the
 *  slave performed and ACK or NACK on the last piece of data sent
 *  to it.
 */
typedef enum {
  I2C_NACK,    ///< The slave has ack-ed the last byte.
  I2C_ACK,     ///< The slave has nack-ed the last byte.
} i2c_res_t;

/** This interface is used to communication with an I2C master component.
 *  It provides facilities for reading and writing to the bus.
 *
 */
typedef interface i2c_master_if {

  /** Write data to an I2C bus.
   *
   *  \param device_addr t   the address of the slave device to write to.
   *  \param buf             the buffer containing data to write.
   *  \param n               the number of bytes to write.
   *  \param num_bytes_sent  the function will set this value to the
   *                         number of bytes actually sent. On success, this
   *                         will be equal to \n but it will be less if the
   *                         slave sends an early NACK on the bus and the
   *                         transaction fails.
   *  \param send_stop_bit   If this is set to non-zero then a stop bit
   *                         will be output on the bus after the transaction.
   *                         This is usually required for normal operation. If
   *                         this parameter is non-zero then no stop bit will
   *                         be omitted. In this case, no other task can use
   *                         the component until either a new read or write
   *                         call is made (a repeated start) or the
   *                         send_stop_bit() function is called.
   *
   *  \returns     whether the write succeeded
   */
  [[guarded]]
  i2c_res_t write(uint8_t device_addr, uint8_t buf[n], size_t n,
               size_t &num_bytes_sent, int send_stop_bit);

  /** Read data from an I2C bus.
   *
   *  \param device_addr     the address of the slave device to read from
   *  \param buf             the buffer to fill with data
   *  \param n               the number of bytes to read
   *  \param send_stop_bit   If this is set to non-zero then a stop bit
   *                         will be output on the bus after the transaction.
   *                         This is usually required for normal operation. If
   *                         this parameter is non-zero then no stop bit will
   *                         be omitted. In this case, no other task can use
   *                         the component until either a new read or write
   *                         call is made (a repeated start) or the
   *                         send_stop_bit() function is called.

   */
  [[guarded]]
  i2c_res_t read(uint8_t device_addr, uint8_t buf[n], size_t n,
               int send_stop_bit);


  /** Send a stop bit.
   *
   *  This function will cause a stop bit to be sent on the bus. It should
   *  be used to complete/abort a transaction if the ``send_stop_bit`` argument
   *  was not set when calling the read() or write() functions.
   */
  void send_stop_bit(void);

  /** Shutdown the I2C component.
   *
   *  This function will cause the I2C task to shutdown and return.
   */
  void shutdown();
} i2c_master_if;

/** This type is used the supplementary I2C register read/write functions to
 *  report back on whether the operation was a success or not.
 */
typedef enum {
  I2C_REGOP_SUCCESS,     ///< The operation was successful
  I2C_REGOP_DEVICE_NACK, ///< The operation was NACK-ed when sending the device address, so either the device is missing or busy.
  I2C_REGOP_INCOMPLETE   ///< The operation was NACK-ed halfway through by the slave.
} i2c_regop_res_t;


extends client interface i2c_master_if : {

  /** Read an 8-bit register on a slave device.
   *
   *  This function reads an 8-bit addressed, 8-bit register from the i2c
   *  bus. The function reads data by
   *  transmitting the register addr and then reading the data from the slave
   *  device.
   *
   *  Note that no stop bit is transmitted between the write and the read.
   *  The operation is performed as one transaction using a repeated start.
   *
   *  \param device_addr the address of the slave device to read from
   *  \param reg         the address of the register to read
   *
   *  \returns           the value of the register
   */
  inline uint8_t read_reg(client interface i2c_master_if i,
                          uint8_t device_addr, uint8_t reg,
                          i2c_regop_res_t &result) {
    uint8_t a_reg[1] = {reg};
    uint8_t data[1] = {0};
    size_t n;
    i2c_res_t res;
    res = i.write(device_addr, a_reg, 1, n, 0);
    if (n != 1) {
      result = I2C_REGOP_DEVICE_NACK;
      i.send_stop_bit();
      return 0;
    }
    res = i.read(device_addr, data, 1, 1);
    if (res == I2C_NACK) {
      result = I2C_REGOP_DEVICE_NACK;
    } else {
      result = I2C_REGOP_SUCCESS;
    }
    return data[0];
  }

  /** Write an 8-bit register on a slave device.
   *
   *  This function writes an 8-bit addressed, 8-bit register from the i2c
   *  bus. The function writes data by
   *  transmitting the register addr and then
   *  transmitting the data to the slave device.
   *
   *  \param device_addr the address of the slave device to write to
   *  \param reg         the address of the register to write
   *  \param data        the 8-bit value to write
   */
  inline i2c_regop_res_t write_reg(client interface i2c_master_if i,
                             uint8_t device_addr, uint8_t reg, uint8_t data)
  {
    uint8_t a_data[2] = {reg, data};
    size_t n;
    i.write(device_addr, a_data, 2, n, 1);
    if (n == 0)
      return I2C_REGOP_DEVICE_NACK;
    if (n < 2)
      return I2C_REGOP_INCOMPLETE;
    return I2C_REGOP_SUCCESS;
  }

  /** Read an 8-bit register on a slave device from a 16 bit register address.
   *
   *  This function reads a 16-bit addressed, 8-bit register from the i2c
   *  bus. The function reads data by
   *  transmitting the register addr and then reading the data from the slave
   *  device.
   *
   *  Note that no stop bit is transmitted between the write and the read.
   *  The operation is performed as one transaction using a repeated start.
   *
   *  \param device_addr the address of the slave device to read from
   *  \param reg         the address of the register to read
   *
   *  \returns           the value of the register
   */
  inline uint8_t read_reg8_addr16(client interface i2c_master_if i,
                                  uint8_t device_addr, uint16_t reg,
                                  i2c_regop_res_t &result)
  {
    uint8_t a_reg[2] = {reg, reg >> 8};
    uint8_t data[1];
    size_t n;
    i2c_res_t res;
    i.write(device_addr, a_reg, 2, n, 0);
    if (n != 2) {
      result = I2C_REGOP_DEVICE_NACK;
      i.send_stop_bit();
      return 0;
    }
    res = i.read(device_addr, data, 1, 1);
    if (res == I2C_NACK) {
      result = I2C_REGOP_DEVICE_NACK;
    } else {
      result = I2C_REGOP_SUCCESS;
    }
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
  inline i2c_regop_res_t write_reg8_addr16(client interface i2c_master_if i,
                                           uint8_t device_addr, uint16_t reg,
                                           uint8_t data) {
    uint8_t a_data[3] = {reg, reg >> 8, data};
    size_t n;
    i.write(device_addr, a_data, 3, n, 1);
    if (n == 0)
      return I2C_REGOP_DEVICE_NACK;
    if (n < 3)
      return I2C_REGOP_INCOMPLETE;
    return I2C_REGOP_SUCCESS;
  }

  /** Read an 16-bit register on a slave device from a 16 bit register address.
   *
   *  This function reads a 16-bit addressed, 16-bit register from the i2c
   *  bus. The function reads data by
   *  transmitting the register addr and then reading the data from the slave
   *  device.
   *
   *  Note that no stop bit is transmitted between the write and the read.
   *  The operation is performed as one transaction using a repeated start.
   *
   *  \param device_addr the address of the slave device to read from
   *  \param reg         the address of the register to read
   *
   *  \returns           the value of the register
   */
  inline uint16_t read_reg16(client interface i2c_master_if i,
                             uint8_t device_addr, uint16_t reg,
                             i2c_regop_res_t &result)
  {
    uint8_t a_reg[2] = {reg, reg >> 8};
    uint8_t data[2];
    size_t n;
    i2c_res_t res;
    i.write(device_addr, a_reg, 2, n, 0);
    if (n != 2) {
      result = I2C_REGOP_DEVICE_NACK;
      i.send_stop_bit();
      return 0;
    }
    res = i.read(device_addr, data, 2, 1);
    if (res == I2C_NACK) {
      result = I2C_REGOP_DEVICE_NACK;
    } else {
      result = I2C_REGOP_SUCCESS;
    }
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
  inline i2c_regop_res_t write_reg16(client interface i2c_master_if i,
                               uint8_t device_addr, uint16_t reg,
                               uint16_t data) {
    uint8_t a_data[4] = {reg, reg >> 8, data, data >> 8};
    size_t n;
    i.write(device_addr, a_data, 4, n, 1);
    if (n == 0)
      return I2C_REGOP_DEVICE_NACK;
    if (n < 4)
      return I2C_REGOP_INCOMPLETE;
    return I2C_REGOP_SUCCESS;
  }


  /** Read an 16-bit register on a slave device from a 8-bit register address.
   *
   *  This function reads a 8-bit addressed, 16-bit register from the i2c
   *  bus. The function reads data by
   *  transmitting the register addr and then reading the data from the slave
   *  device.
   *
   *  Note that no stop bit is transmitted between the write and the read.
   *  The operation is performed as one transaction using a repeated start.
   *
   *  \param device_addr the address of the slave device to read from
   *  \param reg         the address of the register to read
   *
   *  \returns           the value of the register
   */
  inline uint16_t read_reg16_addr8(client interface i2c_master_if i,
                                   uint8_t device_addr, uint8_t reg,
                                   i2c_regop_res_t &result)
  {
    uint8_t a_reg[1] = {reg};
    uint8_t data[2];
    size_t n;
    i2c_res_t res;
    i.write(device_addr, a_reg, 1, n, 0);
    if (n != 1) {
      result = I2C_REGOP_DEVICE_NACK;
      i.send_stop_bit();
      return 0;
    }
    res = i.read(device_addr, data, 2, 1);
    if (res == I2C_NACK) {
      result = I2C_REGOP_DEVICE_NACK;
    } else {
      result = I2C_REGOP_SUCCESS;
    }
    return ((uint16_t) data[0] << 8) | data[1];
  }

  /** Write an 16-bit register on a slave device from a 8-bit register address.
   *
   *  This function writes a 8-bit addressed, 16-bit register from the i2c
   *  bus. The function writes data by
   *  transmitting the register addr and then
   *  transmitting the data to the slave device.
   *
   *  \param device_addr the address of the slave device to write to
   *  \param reg         the address of the register to write
   *  \param data        the 8-bit value to write
   */
  inline i2c_regop_res_t write_reg16_addr8(client interface i2c_master_if i,
                                           uint8_t device_addr, uint8_t reg,
                                           uint16_t data) {
    uint8_t a_data[3] = {reg, data, data >> 8};
    size_t n;
    i.write(device_addr, a_data, 3, n, 1);
    if (n == 0)
      return I2C_REGOP_DEVICE_NACK;
    if (n < 3)
      return I2C_REGOP_INCOMPLETE;
    return I2C_REGOP_SUCCESS;
  }


}


/** Implements I2C on the i2c_master_if interface using two ports.
 *
 *  \param  i      An array of server interface connections for clients to
 *                 connect to
 *  \param  n      The number of clients connected
 *  \param  p_scl  The SCL port of the I2C bus
 *  \param  p_sda  The SDA port of the I2C bus
 *  \param  kbits_per_second The speed of the I2C bus
 **/
[[distributable]] void i2c_master(server interface i2c_master_if i[n],
                                  size_t n,
                                  port p_scl, port p_sda,
                                  unsigned kbits_per_second);

/** Implements I2C on a single multi-bit port.
 *
 *  This function implements an I2C master bus using a single port. However,
 *  If this function is used with an L-series or U-series xCORE device then
 *  reading from the bus and clock stretching are not supported.
 *  The user needs to be aware that these restriction are appropriate for the
 *  application. On xCORE-200 devices, reading and clock stretching are
 *  supported.
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
   *  \param send_stop_bit   If this is set to non-zero then a stop bit
   *                         will be output on the bus after the transaction.
   *                         This is usually required for normal operation. If
   *                         this parameter is non-zero then no stop bit will
   *                         be omitted. In this case, no other task can use
   *                         the component until either a new read or write
   *                         call is made (a repeated start) or the
   *                         send_stop_bit() function is called.
   *
   *
   */
  [[guarded]]
  void write(uint8_t device_addr, uint8_t buf[n], size_t n,
             int send_stop_bit);

  /** Initialize a read to an I2C bus.
   *
   *  \param device_addr     the address of the slave device to read from.
   *  \param n               the number of bytes to read.
   *  \param send_stop_bit   If this is set to non-zero then a stop bit
   *                         will be output on the bus after the transaction.
   *                         This is usually required for normal operation. If
   *                         this parameter is non-zero then no stop bit will
   *                         be omitted. In this case, no other task can use
   *                         the component until either a new read or write
   *                         call is made (a repeated start) or the
   *                         send_stop_bit() function is called.
   *
   */
  [[guarded]]
  void read(uint8_t device_addr, size_t n, int send_stop_bit);

  /** Completed operation notification.
   *
   *  This notification will fire when a read or write is completed.
   */
  [[notification]]
  slave void operation_complete(void);

  /** Get write result.
   *
   *  This function should be called after a write has completed.
   *
   *  \param num_bytes_sent  the function will set this value to the
   *                         number of bytes actually sent. On success, this
   *                         will be equal to \n but it will be less if the
   *                         slave sends an early NACK on the bus and the
   *                         transaction fails.

   *  \returns     whether the write succeeded
   */
  [[clears_notification]]
  i2c_res_t get_write_result(size_t &num_bytes_sent);


  /** Get read result.
   *
   *  This function should be called after a read has completed.
   *
   *  \param buf         the buffer to fill with data.
   *  \param n           the number of bytes to read, this should be the same
   *                     as the number of bytes specified in init_rx(),
   *                     otherwise the behavior is undefined.
   *  \returns           Either ``I2C_SUCCEEDED`` or ``I2C_FAILED`` to indicate
   *                     whether the operation was a success.
   */
  [[clears_notification]]
  i2c_res_t get_read_data(uint8_t buf[n], size_t n);

  /** Send a stop bit.
   *
   *  This function will cause a stop bit to be sent on the bus. It should
   *  be used to complete/abort a transaction if the ``send_stop_bit`` argument
   *  was not set when calling the rx() or write() functions.
   */
  void send_stop_bit(void);


  /** Shutdown the I2C component.
   *
   *  This function will cause the I2C task to shutdown and return.
   */
  void shutdown();
} i2c_master_async_if;

/** I2C master component (asynchronous API).
 *
 *  This function implements I2C and allows clients to asynchronously
 *  perform operations on the bus.
 *
 *  \param   i            the interface to connect to the client of the
 *                        component
 *  \param  p_scl  The SCL port of the I2C bus
 *  \param  p_sda  The SDA port of the I2C bus
 *  \param  kbits_per_second The speed of the I2C bus
 *
 *  \component
 *  \paraminfo     kbits_per_second in [1..400], resources:noeffect
 *  \paraminfo     max_transaction_size resources:linear+orthoganol
 */
void i2c_master_async(server interface i2c_master_async_if i[n],
                      size_t n,
                      port p_scl, port p_sda,
                      unsigned kbits_per_second,
                      static const size_t max_transaction_size);

/** I2C master component (asynchronous API, combinable).
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
void i2c_master_async_comb(server interface i2c_master_async_if i[n],
                           size_t n,
                           port p_scl, port p_sda,
                           unsigned kbits_per_second,
                           static const size_t max_transaction_size);



typedef enum i2c_slave_ack_t {
  I2C_SLAVE_ACK,
  I2C_SLAVE_NACK,
} i2c_slave_ack_t;

/** This interface is used to communication with an I2C slave component.
 *  It provides facilities for reading and writing to the bus. The I2C slave
 *  component acts a *client* to this interface. So the application must
 *  respond to these calls (i.e. the members of the interface are callbacks
 *  to the application).
 *
 */
typedef interface i2c_slave_callback_if {

  /** Start of a read request.
   *
   *  This callback function will be called by the component
   *  if the bus master requests a read from this slave device.
   *  A follow-up call to ack_read_request() will request the
   *  slave to ack the request or not.
   */
  [[guarded]]
  void start_read_request(void);

  /** Master has requested a read.
   *
   *  This callback function will be called by the component
   *  if the bus master requests a read from this slave device after
   *  the start_read_request() call.
   *  At this point the slave can choose to accept the request (and
   *  drive an ACK signal back to the master) or not (and drive a NACK
   *  signal).
   *
   *  \returns  the callback must return either ``I2C_SLAVE_ACK`` or
   *            ``I2C_SLAVE_NACK``.
   */
  [[guarded]]
  i2c_slave_ack_t ack_read_request(void);

  /** Start of a write request.
   *
   *  This callback function will be called by the component
   *  if the bus master requests a write from this slave device.
   *  A follow-up call to ack_write_request() will request the
   *  slave to ack the request or not.
   */
  [[guarded]]
  void start_write_request(void);

  /** Master has requested a write.
   *
   *  This callback function will be called by the component
   *  if the bus master requests a write from this slave device after
   *  the start_write_request() call.
   *  At this point the slave can choose to accept the request (and
   *  drive an ACK signal back to the master) or not (and drive a NACK
   *  signal).
   *
   *  \returns  the callback must return either ``I2C_SLAVE_ACK`` or
   *            ``I2C_SLAVE_NACK``.
   */
  [[guarded]]
  i2c_slave_ack_t ack_write_request(void);


  /** Start of a data read.
   *
   *  This callback function will be called at the start of a byte read.
   */
  [[guarded]]
  void start_master_read(void);

  /** Master requires data.
   *
   *  This callback function will be called when the I2C master requires data
   *  from the slave.
   *
   *  \return   the data to pass to the master.
   */
  [[guarded]]
  uint8_t master_requires_data();

  /** Start of a data write.
   *
   *  This callback function will be called at the start of writing a byte.
   */
  [[guarded]]
  void start_master_write(void);

  /** Master has sent some data.
   *
   *  This callback function will be called when the I2C master has transferred
   *  a byte of data to the slave.
   */
  [[guarded]]
  i2c_slave_ack_t master_sent_data(uint8_t data);


  /** Stop bit.
   *
   *  This callback function will be called by the component when a stop bit
   *  is sent by the master.
   */
  void stop_bit(void);

  /** Shutdown the I2C component.
   *
   *  This function will cause the I2C slave task to shutdown and return.
   */
  [[notification]] slave void shutdown();
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
               uint8_t device_addr);

#endif
