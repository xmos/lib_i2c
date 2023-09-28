// Copyright 2014-2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef _i2c_master_if_h_
#define _i2c_master_if_h_

#include <i2c_common.h>

#include <stddef.h>
#include <stdint.h>

#ifdef __XC__

#define BIT_TIME(KBITS_PER_SEC) ((XS1_TIMER_MHZ * 1000) / KBITS_PER_SEC)
#define BIT_MASK(BIT_POS) (1 << BIT_POS)

/** This interface is used to communication with an I2C master component.
 *  It provides facilities for reading and writing to the bus.
 *
 */
typedef interface i2c_master_if {

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
               size_t &num_bytes_sent, int send_stop_bit);

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
} i2c_master_if;


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
  inline i2c_regop_res_t write_reg(client interface i2c_master_if i,
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
  inline uint8_t read_reg8_addr16(client interface i2c_master_if i,
                                  uint8_t device_addr, uint16_t reg,
                                  i2c_regop_res_t &result)
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
  inline i2c_regop_res_t write_reg8_addr16(client interface i2c_master_if i,
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
  inline uint16_t read_reg16(client interface i2c_master_if i,
                             uint8_t device_addr, uint16_t reg,
                             i2c_regop_res_t &result)
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
  inline i2c_regop_res_t write_reg16(client interface i2c_master_if i,
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
  inline i2c_regop_res_t write_reg16_addr8(client interface i2c_master_if i,
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
}

/** Implements I2C on the i2c_master_if interface using two ports.
 *
 *  \param  i                an array of server interface connections for clients
 *                           to connect to
 *  \param  n                the number of clients connected
 *  \param  p_scl            the SCL port of the I2C bus
 *  \param  p_sda            the SDA port of the I2C bus
 *  \param  kbits_per_second the speed of the I2C bus
 **/
[[distributable]] void i2c_master(server interface i2c_master_if i[n],
                                  size_t n,
                                  port p_scl, port p_sda,
                                  static const unsigned kbits_per_second);

#endif // __XC__

#endif
