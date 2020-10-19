// Copyright (c) 2020, XMOS Ltd, All rights reserved
#ifndef _i2c_c_reg_h_
#define _i2c_c_reg_h_

#include <string.h>
#include "i2c.h"

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
 *  \param ctx         the I2C master context to use
 *  \param device_addr the address of the slave device to read from
 *  \param reg         the address of the register to read
 *  \param result      indicates whether the read completed successfully. Will
 *                     be set to ``I2C_REGOP_DEVICE_NACK`` if the slave NACKed,
 *                     and ``I2C_REGOP_SUCCESS`` on successful completion of the
 *                     read.
 *
 *  \returns           the value of the register
 */
inline uint8_t read_reg(
        i2c_master_t *ctx,
        uint8_t device_addr,
        uint8_t reg,
        i2c_regop_res_t *result) {
    uint8_t buf[1] = {reg};
    size_t bytes_sent = 0;
    i2c_res_t res;

    res = i2c_master_write(ctx, device_addr, buf, 1, &bytes_sent, 0);
    if (bytes_sent != 1) {
        *result = I2C_REGOP_DEVICE_NACK;
        i2c_master_stop_bit_send(ctx);
        return 0;
    }
    memset(buf, 0x00, 1);
    res = i2c_master_read(ctx, device_addr, buf, 1, 1);
    if (res == I2C_NACK) {
        *result = I2C_REGOP_DEVICE_NACK;
    } else {
        *result = I2C_REGOP_SUCCESS;
    }
    return buf[0];
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
 *  \param ctx         the I2C master context to use
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
inline uint8_t read_reg8_addr16(
        i2c_master_t *ctx,
        uint8_t device_addr,
        uint16_t reg,
        i2c_regop_res_t *result) {
    uint8_t buf[2] = {(reg >> 8) & 0xFF, reg & 0xFF};
    size_t bytes_sent = 0;
    i2c_res_t res;

    res = i2c_master_write(ctx, device_addr, buf, 2, &bytes_sent, 0);
    if (bytes_sent != 2) {
        *result = I2C_REGOP_DEVICE_NACK;
        i2c_master_stop_bit_send(ctx);
        return 0;
    }
    memset(buf, 0x00, 2);
    res = i2c_master_read(ctx, device_addr, buf, 1, 1);
    if (res == I2C_NACK) {
        *result = I2C_REGOP_DEVICE_NACK;
    } else {
        *result = I2C_REGOP_SUCCESS;
    }
    return buf[0];
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
 *  \param ctx         the I2C master context to use
 *  \param device_addr the address of the slave device to read from
 *  \param reg         the address of the register to read
 *  \param result      indicates whether the read completed successfully. Will
 *                     be set to ``I2C_REGOP_DEVICE_NACK`` if the slave NACKed,
 *                     and ``I2C_REGOP_SUCCESS`` on successful completion of the
 *                     read.
 *
 *  \returns           the 16-bit value of the register
 */
inline uint16_t read_reg16_addr8(
        i2c_master_t *ctx,
        uint8_t device_addr,
        uint8_t reg,
        i2c_regop_res_t *result) {
    uint8_t buf[2] = {reg, 0x00};
    size_t bytes_sent = 0;
    i2c_res_t res;

    res = i2c_master_write(ctx, device_addr, buf, 1, &bytes_sent, 0);
    if (bytes_sent != 1) {
        *result = I2C_REGOP_DEVICE_NACK;
        i2c_master_stop_bit_send(ctx);
        return 0;
    }
    memset(buf, 0x00, 2);
    res = i2c_master_read(ctx, device_addr, buf, 2, 1);
    if (res == I2C_NACK) {
        *result = I2C_REGOP_DEVICE_NACK;
    } else {
        *result = I2C_REGOP_SUCCESS;
    }
    return (uint16_t)((buf[0] << 8 )| buf[1]);
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
 *  \param ctx         the I2C master context to use
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
inline uint16_t read_reg16(
        i2c_master_t *ctx,
        uint8_t device_addr,
        uint16_t reg,
        i2c_regop_res_t *result) {
    uint8_t buf[2] = {(reg >> 8) & 0xFF, reg & 0xFF};
    size_t bytes_sent = 0;
    i2c_res_t res;

    res = i2c_master_write(ctx, device_addr, buf, 2, &bytes_sent, 0);
    if (bytes_sent != 2) {
        *result = I2C_REGOP_DEVICE_NACK;
        i2c_master_stop_bit_send(ctx);
        return 0;
    }
    memset(buf, 0x00, 2);
    res = i2c_master_read(ctx, device_addr, buf, 2, 1);
    if (res == I2C_NACK) {
        *result = I2C_REGOP_DEVICE_NACK;
    } else {
        *result = I2C_REGOP_SUCCESS;
    }
    return (uint16_t)((buf[0] << 8 )| buf[1]);
}

/** Write an 8-bit register on a slave device.
 *
 *  This function writes an 8-bit addressed, 8-bit register from the i2c
 *  bus. The function writes data by
 *  transmitting the register addr and then
 *  transmitting the data to the slave device.
 *
 *  \param ctx         the I2C master context to use
 *  \param device_addr the address of the slave device to write to
 *  \param reg         the address of the register to write
 *  \param data        the 8-bit value to write
 */
inline i2c_regop_res_t write_reg(
        i2c_master_t *ctx,
        uint8_t device_addr,
        uint8_t reg,
        uint8_t data) {
    uint8_t buf[2] = {reg, data};
    size_t bytes_sent = 0;
    i2c_regop_res_t reg_res;

    i2c_master_write(ctx, device_addr, buf, 2, &bytes_sent, 1);
    if (bytes_sent == 0) {
        reg_res = I2C_REGOP_DEVICE_NACK;
    } else if (bytes_sent < 2) {
        reg_res = I2C_REGOP_INCOMPLETE;
    } else {
        reg_res = I2C_REGOP_SUCCESS;
    }
    return reg_res;
}

/** Write an 8-bit register on a slave device from a 16-bit register address.
 *
 *  This function writes a 16-bit addressed, 8-bit register from the i2c
 *  bus. The function writes data by
 *  transmitting the register addr and then
 *  transmitting the data to the slave device.
 *
 *  \param ctx         the I2C master context to use
 *  \param device_addr the address of the slave device to write to
 *  \param reg         the 16-bit address of the register to write
 *                     (most significant byte first)
 *  \param data        the 8-bit value to write
 */
inline i2c_regop_res_t write_reg8_addr16(
        i2c_master_t *ctx,
        uint8_t device_addr,
        uint16_t reg,
        uint8_t data) {
    uint8_t buf[3] = {(reg >> 8) & 0xFF, reg & 0xFF, data};
    size_t bytes_sent = 0;
    i2c_regop_res_t reg_res;

    i2c_master_write(ctx, device_addr, buf, 3, &bytes_sent, 1);
    if (bytes_sent == 0) {
        reg_res = I2C_REGOP_DEVICE_NACK;
    } else if (bytes_sent < 3) {
        reg_res = I2C_REGOP_INCOMPLETE;
    } else {
        reg_res = I2C_REGOP_SUCCESS;
    }
    return reg_res;
}

/** Write an 16-bit register on a slave device from a 8-bit register address.
 *
 *  This function writes a 8-bit addressed, 16-bit register from the i2c
 *  bus. The function writes data by transmitting the register addr and then
 *  transmitting the data to the slave device.
 *
 *  \param ctx         the I2C master context to use
 *  \param device_addr the address of the slave device to write to
 *  \param reg         the address of the register to write
 *  \param data        the 16-bit value to write (most significant byte first)
 *
 *  \returns           ``I2C_REGOP_DEVICE_NACK`` if the address is NACKed,
 *                     ``I2C_REGOP_INCOMPLETE`` if not all data was ACKed and
 *                     ``I2C_REGOP_SUCCESS`` on successful completion of the
 *                     write with every byte being ACKed.
 */
inline i2c_regop_res_t write_reg16_addr8(
        i2c_master_t *ctx,
        uint8_t device_addr,
        uint8_t reg,
        uint16_t data) {
    uint8_t buf[3] = {reg, (data >> 8) & 0xFF, data & 0xFF};
    size_t bytes_sent = 0;
    i2c_regop_res_t reg_res;

    i2c_master_write(ctx, device_addr, buf, 3, &bytes_sent, 1);
    if (bytes_sent == 0) {
        reg_res = I2C_REGOP_DEVICE_NACK;
    } else if (bytes_sent < 3) {
        reg_res = I2C_REGOP_INCOMPLETE;
    } else {
        reg_res = I2C_REGOP_SUCCESS;
    }
    return reg_res;
}

/** Write an 16-bit register on a slave device from a 16-bit register address.
 *
 *  This function writes a 16-bit addressed, 16-bit register from the i2c
 *  bus. The function writes data by transmitting the register addr and then
 *  transmitting the data to the slave device.
 *
 *  \param ctx         the I2C master context to use
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
inline i2c_regop_res_t write_reg16(
        i2c_master_t *ctx,
        uint8_t device_addr,
        uint16_t reg,
        uint16_t data) {
    uint8_t buf[4] = {(reg >> 8) & 0xFF, reg & 0xFF, (data >> 8) & 0xFF, data & 0xFF};
    size_t bytes_sent = 0;
    i2c_regop_res_t reg_res;

    i2c_master_write(ctx, device_addr, buf, 4, &bytes_sent, 1);
    if (bytes_sent == 0) {
        reg_res = I2C_REGOP_DEVICE_NACK;
    } else if (bytes_sent < 4) {
        reg_res = I2C_REGOP_INCOMPLETE;
    } else {
        reg_res = I2C_REGOP_SUCCESS;
    }
    return reg_res;
}

#endif
