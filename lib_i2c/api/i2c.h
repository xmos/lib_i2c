// Copyright 2014-2024 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef _i2c_h_
#define _i2c_h_

#include <stddef.h>
#include <stdint.h>
#include <xccompat.h>

// These are needed for DOXYGEN to render properly
#ifndef __DOXYGEN__
#define static_const_unsigned static const unsigned
#define static_const_size_t static const size_t
#define port_t port
#define async_master_shutdown shutdown
#define async_master_send_stop_bit send_stop_bit
#define async_master_read read
#define async_master_write write
#define slave_void  slave void
#endif

/** This type is used in I2C functions to report back on whether the
 *  slave performed an ACK or NACK on the last piece of data sent
 *  to it.
 */
typedef enum {
  I2C_NACK,    ///< the slave has NACKed the last byte
  I2C_ACK,     ///< the slave has ACKed the last byte
} i2c_res_t;

#if(defined __XC__ || defined __DOXYGEN__)

#define BIT_TIME(KBITS_PER_SEC) ((XS1_TIMER_MHZ * 1000) / KBITS_PER_SEC)
#define BIT_MASK(BIT_POS) (1 << BIT_POS)

/** This interface is used to communication with an I2C master component.
 *  It provides facilities for reading and writing to the bus.
 *
 */
#ifndef __DOXYGEN__
typedef interface i2c_master_if {
#endif
  /**
   * \addtogroup i2c_master_if
   * @{
   */

  /** Write data to an I2C bus.
   *
   *  \param device_addr     the address of the slave device to write to.
   *  \param buf             the buffer containing data to write.
   *  \param n               the number of bytes to write.
   *  \param num_bytes_sent  the function will set this value to the
   *                         number of bytes actually sent. On success, this
   *                         will be equal to ``n`` but it will be less if the
   *                         slave sends an early NACK on the bus and the
   *                         transaction fails.
   *  \param send_stop_bit   if this is non-zero then a stop bit
   *                         will be sent on the bus after the transaction.
   *                         This is usually required for normal operation. If
   *                         this parameter is zero then no stop bit will
   *                         be omitted. In this case, no other task can use
   *                         the component until a stop bit has been sent.
   *
   *  \returns               ``I2C_ACK`` if the write was acknowledged by the slave
   *                         device, otherwise ``I2C_NACK``.
   */
  [[guarded]]
  i2c_res_t write(uint8_t device_addr, uint8_t buf[n], size_t n,
               REFERENCE_PARAM(size_t, num_bytes_sent), int send_stop_bit);

  /** Read data from an I2C bus.
   *
   *  \param device_addr     the address of the slave device to read from
   *  \param buf             the buffer to fill with data
   *  \param n               the number of bytes to read
   *  \param send_stop_bit   if this is non-zero then a stop bit
   *                         will be sent on the bus after the transaction.
   *                         This is usually required for normal operation. If
   *                         this parameter is zero then no stop bit will
   *                         be omitted. In this case, no other task can use
   *                         the component until a stop bit has been sent.
   *
   *  \returns               ``I2C_ACK`` if the read was acknowledged by the slave
   *                         device, otherwise ``I2C_NACK``.
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

/**@}*/ // END: addtogroup i2c_master_if

#ifndef __DOXYGEN__
} i2c_master_if;
#endif

/** This type is used by the supplementary I2C register read/write functions to
 *  report back on whether the operation was a success or not.
 */
typedef enum {
  I2C_REGOP_SUCCESS,     ///< the operation was successful
  I2C_REGOP_DEVICE_NACK, ///< the operation was NACKed when sending the device address, so either the device is missing or busy
  I2C_REGOP_INCOMPLETE   ///< the operation was NACKed halfway through by the slave
} i2c_regop_res_t;

#ifndef __DOXYGEN__
extends client interface i2c_master_if : {
#endif
  /**
   * \addtogroup i2c_master_if
   * @{
   */
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
   *  \param i           the interface to the I2C master
   *  \param device_addr the address of the slave device to read from
   *  \param reg         the address of the register to read
   *  \param result      indicates whether the read completed successfully. Will
   *                     be set to ``I2C_REGOP_DEVICE_NACK`` if the slave NACKed,
   *                     and ``I2C_REGOP_SUCCESS`` on successful completion of the
   *                     read.
   *
   *  \returns           the value of the register
   */
  inline uint8_t read_reg(CLIENT_INTERFACE(i2c_master_if, i),
                          uint8_t device_addr, uint8_t reg,
                          REFERENCE_PARAM(i2c_regop_res_t, result)) {
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
    if (res == I2C_ACK) {
      result = I2C_REGOP_SUCCESS;
    } else {
      result = I2C_REGOP_DEVICE_NACK;
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
   *  \param i           the interface to the I2C master
   *  \param device_addr the address of the slave device to write to
   *  \param reg         the address of the register to write
   *  \param data        the 8-bit value to write
   */
  inline i2c_regop_res_t write_reg(CLIENT_INTERFACE(i2c_master_if, i),
                             uint8_t device_addr, uint8_t reg, uint8_t data)
  {
    uint8_t a_data[2] = {reg, data};
    size_t n;
    i.write(device_addr, a_data, 2, n, 1);
    if (n == 0) {
      return I2C_REGOP_DEVICE_NACK;
    }
    if (n < 2) {
      return I2C_REGOP_INCOMPLETE;
    }
    return I2C_REGOP_SUCCESS;
  }

  /** Read an 8-bit register on a slave device from a 16-bit register address.
   *
   *  This function reads a 16-bit addressed, 8-bit register from the i2c
   *  bus. The function reads data by
   *  transmitting the register addr and then reading the data from the slave
   *  device.
   *
   *  Note that no stop bit is transmitted between the write and the read.
   *  The operation is performed as one transaction using a repeated start.
   *
   *  \param i           the interface to the I2C master
   *  \param device_addr the address of the slave device to read from
   *  \param reg         the 16-bit address of the register to read
   *                     (most significant byte first)
   *  \param result      indicates whether the read completed successfully. Will
   *                     be set to ``I2C_REGOP_DEVICE_NACK`` if the slave NACKed,
   *                     and ``I2C_REGOP_SUCCESS`` on successful completion of the
   *                     read.
   *
   *  \returns           the value of the register
   */
  inline uint8_t read_reg8_addr16(CLIENT_INTERFACE(i2c_master_if, i),
                                  uint8_t device_addr, uint16_t reg,
                                  REFERENCE_PARAM(i2c_regop_res_t, result))
  {
    uint8_t a_reg[2] = {reg >> 8, reg};
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

  /** Write an 8-bit register on a slave device from a 16-bit register address.
   *
   *  This function writes a 16-bit addressed, 8-bit register from the i2c
   *  bus. The function writes data by
   *  transmitting the register addr and then
   *  transmitting the data to the slave device.
   *
   *  \param i           the interface to the I2C master
   *  \param device_addr the address of the slave device to write to
   *  \param reg         the 16-bit address of the register to write
   *                     (most significant byte first)
   *  \param data        the 8-bit value to write
   */
  inline i2c_regop_res_t write_reg8_addr16(CLIENT_INTERFACE(i2c_master_if, i),
                                           uint8_t device_addr, uint16_t reg,
                                           uint8_t data) {
    uint8_t a_data[3] = {reg >> 8, reg, data};
    size_t n;
    i.write(device_addr, a_data, 3, n, 1);
    if (n == 0) {
      return I2C_REGOP_DEVICE_NACK;
    }
    if (n < 3) {
      return I2C_REGOP_INCOMPLETE;
    }
    return I2C_REGOP_SUCCESS;
  }

  /** Read an 16-bit register on a slave device from a 16-bit register address.
   *
   *  This function reads a 16-bit addressed, 16-bit register from the i2c
   *  bus. The function reads data by transmitting the register addr and then
   *  reading the data from the slave device. It is assumed the data is returned
   *  most significant byte first on the bus.
   *
   *  Note that no stop bit is transmitted between the write and the read.
   *  The operation is performed as one transaction using a repeated start.
   *
   *  \param i           the interface to the I2C master
   *  \param device_addr the address of the slave device to read from
   *  \param reg         the address of the register to read (most
   *                     significant byte first)
   *  \param result      indicates whether the read completed successfully. Will
   *                     be set to ``I2C_REGOP_DEVICE_NACK`` if the slave NACKed,
   *                     and ``I2C_REGOP_SUCCESS`` on successful completion of the
   *                     read.
   *
   *  \returns           the 16-bit value of the register
   */
  inline uint16_t read_reg16(CLIENT_INTERFACE(i2c_master_if, i),
                             uint8_t device_addr, uint16_t reg,
                             REFERENCE_PARAM(i2c_regop_res_t, result))
  {
    uint8_t a_reg[2] = {reg >> 8, reg};
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

  /** Write an 16-bit register on a slave device from a 16-bit register address.
   *
   *  This function writes a 16-bit addressed, 16-bit register from the i2c
   *  bus. The function writes data by transmitting the register addr and then
   *  transmitting the data to the slave device.
   *
   *  \param i           the interface to the I2C master
   *  \param device_addr the address of the slave device to write to
   *  \param reg         the 16-bit address of the register to write
   *                     (most significant byte first)
   *  \param data        the 16-bit value to write (most significant
   *                     byte first)
   *
   *  \returns           ``I2C_REGOP_DEVICE_NACK`` if the address is NACKed,
   *                     ``I2C_REGOP_INCOMPLETE`` if not all data was ACKed and
   *                     ``I2C_REGOP_SUCCESS`` on successful completion of the
   *                     write with every byte being ACKed.
   */
  inline i2c_regop_res_t write_reg16(CLIENT_INTERFACE(i2c_master_if, i),
                               uint8_t device_addr, uint16_t reg,
                               uint16_t data) {
    uint8_t a_data[4] = {reg >> 8, reg, data >> 8, data};
    size_t n;
    i.write(device_addr, a_data, 4, n, 1);
    if (n == 0) {
      return I2C_REGOP_DEVICE_NACK;
    }
    if (n < 4) {
      return I2C_REGOP_INCOMPLETE;
    }
    return I2C_REGOP_SUCCESS;
  }

  /** Read an 16-bit register on a slave device from a 8-bit register address.
   *
   *  This function reads a 8-bit addressed, 16-bit register from the i2c
   *  bus. The function reads data by  transmitting the register addr and
   *  then reading the data from the slave device. It is assumed that the data
   *  is return most significant byte first on the bus.
   *
   *  Note that no stop bit is transmitted between the write and the read.
   *  The operation is performed as one transaction using a repeated start.
   *
   *  \param i           the interface to the I2C master
   *  \param device_addr the address of the slave device to read from
   *  \param reg         the address of the register to read
   *  \param result      indicates whether the read completed successfully. Will
   *                     be set to ``I2C_REGOP_DEVICE_NACK`` if the slave NACKed,
   *                     and ``I2C_REGOP_SUCCESS`` on successful completion of the
   *                     read.
   *
   *  \returns           the 16-bit value of the register
   */
  inline uint16_t read_reg16_addr8(CLIENT_INTERFACE(i2c_master_if, i),
                                   uint8_t device_addr, uint8_t reg,
                                   REFERENCE_PARAM(i2c_regop_res_t, result))
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
   *  bus. The function writes data by transmitting the register addr and then
   *  transmitting the data to the slave device.
   *
   *  \param i           the interface to the I2C master
   *  \param device_addr the address of the slave device to write to
   *  \param reg         the address of the register to write
   *  \param data        the 16-bit value to write (most significant byte first)
   *
   *  \returns           ``I2C_REGOP_DEVICE_NACK`` if the address is NACKed,
   *                     ``I2C_REGOP_INCOMPLETE`` if not all data was ACKed and
   *                     ``I2C_REGOP_SUCCESS`` on successful completion of the
   *                     write with every byte being ACKed.
   */
  inline i2c_regop_res_t write_reg16_addr8(CLIENT_INTERFACE(i2c_master_if, i),
                                           uint8_t device_addr, uint8_t reg,
                                           uint16_t data) {
    uint8_t a_data[3] = {reg, data >> 8, data};
    size_t n;
    i.write(device_addr, a_data, 3, n, 1);
    if (n == 0) {
      return I2C_REGOP_DEVICE_NACK;
    }
    if (n < 3) {
      return I2C_REGOP_INCOMPLETE;
    }
    return I2C_REGOP_SUCCESS;
  }

/**@}*/ // END: addtogroup i2c_master_if

#ifndef __DOXYGEN__
}
#endif

/** Implements I2C on the i2c_master_if interface using two ports.
 *
 *  \param  i                an array of server interface connections for clients
 *                           to connect to
 *  \param  n                the number of clients connected
 *  \param  p_scl            the SCL port of the I2C bus
 *  \param  p_sda            the SDA port of the I2C bus
 *  \param  kbits_per_second the speed of the I2C bus
 **/
[[distributable]]
void i2c_master(SERVER_INTERFACE(i2c_master_if, i[n]),
                                  size_t n,
                                  port_t p_scl, port_t p_sda,
                                  static_const_unsigned kbits_per_second);

#if (defined(__XS2A__) || defined(__XS3A__) || defined(__DOXYGEN__))
/** Implements I2C on a single multi-bit port.
 *
 *  This function implements an I2C master bus using a single port. Not supported on L or U series
 *  devices.
 *
 *  \param  c                an array of server interface connections for clients
 *                           to connect to
 *  \param  n                the number of clients connected
 *  \param  p_i2c            the multi-bit port containing both SCL and SDA.
 *                           the bit positions of SDA and SCL are configured using the
 *                           ``sda_bit_position`` and ``scl_bit_position`` arguments.
 *  \param  kbits_per_second the speed of the I2C bus
 *  \param  sda_bit_position the bit of the SDA line on the port
 *  \param  scl_bit_position the bit of the SCL line on the port
 *  \param  other_bits_mask  a value that is ORed into the port value driven
 *                           to ``p_i2c``. The SDA and SCL bit values for this
 *                           variable must be set to 0. Note that ``p_i2c`` is
 *                           configured with set_port_drive_low() and
 *                           therefore external pullup resistors are required
 *                           to produce a value 1 on a bit.
 */
[[distributable]]
void i2c_master_single_port(SERVER_INTERFACE(i2c_master_if, c[n]), static_const_size_t n,
                            port_t p_i2c, static_const_unsigned kbits_per_second,
                            static_const_unsigned scl_bit_position,
                            static_const_unsigned sda_bit_position,
                            static_const_unsigned other_bits_mask);
#endif


/** This interface is used to communicate with an I2C master component
 *  asynchronously.
 *  It provides facilities for reading and writing to the bus.
 *
 */
#ifndef __DOXYGEN__
typedef interface i2c_master_async_if {
#endif

  /**
   * \addtogroup i2c_master_async_if
   * @{
   */

  /** Initialize a write to an I2C bus.
   *
   *  \param device_addr     the address of the slave device to write to
   *  \param buf             the buffer containing data to write
   *  \param n               the number of bytes to write
   *  \param send_stop_bit   if this is non-zero then a stop bit
   *                         will be sent on the bus after the transaction.
   *                         This is usually required for normal operation. If
   *                         this parameter is zero then no stop bit will
   *                         be omitted. In this case, no other task can use
   *                         the component until a stop bit has been sent.
   */
  [[guarded]]
  void async_master_write(uint8_t device_addr, uint8_t buf[n], size_t n,
             int send_stop_bit);

  /** Initialize a read to an I2C bus.
   *
   *  \param device_addr     the address of the slave device to read from.
   *  \param n               the number of bytes to read.
   *  \param send_stop_bit   if this is non-zero then a stop bit
   *                         will be sent on the bus after the transaction.
   *                         This is usually required for normal operation. If
   *                         this parameter is zero then no stop bit will
   *                         be omitted. In this case, no other task can use
   *                         the component until a stop bit has been sent.
   */
  [[guarded]]
  void async_master_read(uint8_t device_addr, size_t n, int send_stop_bit);

  /** Completed operation notification.
   *
   *  This notification will fire when a read or write is completed.
   */
  [[notification]]
  slave_void operation_complete(void);

  /** Get write result.
   *
   *  This function should be called after a write has completed.
   *
   *  \param num_bytes_sent  the function will set this value to the
   *                         number of bytes actually sent. On success, this
   *                         will be equal to ``n`` but it will be less if the
   *                         slave sends an early NACK on the bus and the
   *                         transaction fails.
   *
   *  \returns               ``I2C_ACK`` if the write was acknowledged by the slave
   *                         device, otherwise ``I2C_NACK``.
   */
  [[clears_notification]]
  i2c_res_t get_write_result(REFERENCE_PARAM(size_t, num_bytes_sent));


  /** Get read result.
   *
   *  This function should be called after a read has completed.
   *
   *  \param buf         the buffer to fill with data.
   *  \param n           the number of bytes to read, this should be the same
   *                     as the number of bytes specified in read(),
   *                     otherwise the behavior is undefined.
   *
   *  \returns           ``I2C_ACK`` if the write was acknowledged by the slave
   *                     device, otherwise ``I2C_NACK``.
   */
  [[clears_notification]]
  i2c_res_t get_read_data(uint8_t buf[n], size_t n);

  /** Send a stop bit.
   *
   *  This function will cause a stop bit to be sent on the bus. It should
   *  be used to complete/abort a transaction if the ``send_stop_bit`` argument
   *  was not set when calling the read() or write() functions.
   */
  void async_master_send_stop_bit(void);


  /** Shutdown the I2C component.
   *
   *  This function will cause the I2C task to shutdown and return.
   */
  void async_master_shutdown();

/**@}*/ // END: addtogroup i2c_master_async_if

#ifndef __DOXYGEN__
} i2c_master_async_if;
#endif

/** I2C master component (asynchronous API).
 *
 *  This function implements I2C and allows clients to asynchronously
 *  perform operations on the bus.
 *
 *  \param  i                    the interfaces to connect the component to its clients
 *  \param  n                    the number of clients connected to the component
 *  \param  p_scl                the SCL port of the I2C bus
 *  \param  p_sda                the SDA port of the I2C bus
 *  \param  kbits_per_second     the speed of the I2C bus
 *  \param  max_transaction_size the size of the local buffer in bytes. Any
 *                               transactions exceeding this size will cause a
 *                               run-time exception.
 *
 */
void i2c_master_async(SERVER_INTERFACE(i2c_master_async_if, i[n]),
                      size_t n,
                      port_t p_scl, port_t p_sda,
                      static_const_unsigned kbits_per_second,
                      static_const_size_t max_transaction_size);

/** I2C master component (asynchronous API, combinable).
 *
 *  This function implements I2C and allows clients to asynchronously
 *  perform operations on the bus.
 *  Note that this component can be run on the same logical core as other
 *  tasks (i.e. it is [[combinable]]). However, care must be taken that the
 *  other tasks do not take too long in their select cases otherwise this
 *  component may miss I2C transactions.
 *
 *  \param  i                    the interfaces to connect the component to its clients
 *  \param  n                    the number of clients connected to the component
 *  \param  p_scl                the SCL port of the I2C bus
 *  \param  p_sda                the SDA port of the I2C bus
 *  \param  kbits_per_second     the speed of the I2C bus
 *  \param  max_transaction_size the size of the local buffer in bytes. Any
 *                               transactions exceeding this size will cause a
 *                               run-time exception.
 */
[[combinable]]
void i2c_master_async_comb(SERVER_INTERFACE(i2c_master_async_if, i[n]),
                           size_t n,
                           port_t p_scl, port_t p_sda,
                           static_const_unsigned kbits_per_second,
                           static_const_size_t max_transaction_size);



typedef enum i2c_slave_ack_t {
  I2C_SLAVE_ACK,
  I2C_SLAVE_NACK,
} i2c_slave_ack_t;

/** This interface is used to communicate with an I2C slave component.
 *  It provides facilities for reading and writing to the bus. The I2C slave
 *  component acts a *client* to this interface. So the application must
 *  respond to these calls (i.e. the members of the interface are callbacks
 *  to the application).
 *
 */
#ifndef __DOXYGEN__
typedef interface i2c_slave_callback_if {
#endif

  /**
   * \addtogroup i2c_slave_callback_if
   * @{
   */

  /** Master has requested a read.
   *
   *  This callback function is called by the component
   *  if the bus master requests a read from this slave device.
   *
   *  At this point the slave can choose to accept the request (and
   *  drive an ACK signal back to the master) or not (and drive a NACK
   *  signal).
   *
   *  \returns  the callback must return either ``I2C_SLAVE_ACK`` or
   *            ``I2C_SLAVE_NACK``.
   */
  [[guarded]]
  i2c_slave_ack_t ack_read_request(void);


  /** Master has requested a write.
   *
   *  This callback function is called by the component
   *  if the bus master requests a write from this slave device.
   *
   *  At this point the slave can choose to accept the request (and
   *  drive an ACK signal back to the master) or not (and drive a NACK
   *  signal).
   *
   *  \returns  the callback must return either ``I2C_SLAVE_ACK`` or
   *            ``I2C_SLAVE_NACK``.
   */
  [[guarded]]
  i2c_slave_ack_t ack_write_request(void);

  /** Master requires data.
   *
   *  This callback function will be called when the I2C master requires data
   *  from the slave.
   *
   *  \return   the data to pass to the master.
   */
  [[guarded]]
  uint8_t master_requires_data();

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

  /**@}*/ // END: addtogroup i2c_slave_callback_if
#ifndef __DOXYGEN__
} i2c_slave_callback_if;
#endif


/** I2C slave task.
 *
 *  This function instantiates an i2c_slave component.
 *
 *  \param i           the client end of the i2c_slave_callback_if interface. The component
 *                     takes the client end and will make calls on the interface when
 *                     the master performs reads or writes.
 *  \param  p_scl      the SCL port of the I2C bus
 *  \param  p_sda      the SDA port of the I2C bus
 *  \param device_addr the address of the slave device
 *
 */
[[combinable]]
void i2c_slave(CLIENT_INTERFACE(i2c_slave_callback_if, i),
               port_t p_scl, port_t p_sda,
               uint8_t device_addr);

#endif // __XC__

#endif
