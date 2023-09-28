// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#ifndef _i2c_h_
#define _i2c_h_

#include <i2c_common.h>

#include <sys/types.h>

#ifndef __XC__
#include <xcore/port.h>
#include <xcore/clock.h>
#include <xcore/hwtimer.h>
#endif

#include <xccompat.h>

/**
 * \addtogroup hil_i2c_master hil_i2c_master
 *
 * The public API for using the RTOS I2C slave driver.
 * @{
 */

/**
 * Type representing an I2C master context
 */
typedef struct i2c_master_struct i2c_master_t;

/**
 * Struct to hold an I2C master context.
 *
 * The members in this struct should not be accessed directly.
 */
struct i2c_master_struct {
    port_t p_scl;
    uint32_t scl_mask;
    port_t p_sda;
    uint32_t sda_mask;

    uint32_t scl_high;
    uint32_t sda_high;
    uint32_t scl_low;
    uint32_t sda_low;

    uint16_t bit_time;
    uint16_t p_setup_ticks;
    uint16_t sr_setup_ticks;
    uint16_t s_hold_ticks;
    uint16_t low_period_ticks;
    uint16_t high_period_ticks;

    int interrupt_state;
    int stopped;
};

#ifndef __XC__

/**
 * Writes data to an I2C bus as a master.
 *
 * \param ctx             A pointer to the I2C master context to use.
 * \param device_addr     The address of the device to write to.
 * \param buf             The buffer containing data to write.
 * \param n               The number of bytes to write.
 * \param num_bytes_sent  The function will set this value to the
 *                        number of bytes actually sent. On success, this
 *                        will be equal to ``n`` but it will be less if the
 *                        slave sends an early NACK on the bus and the
 *                        transaction fails.
 * \param send_stop_bit   If this is non-zero then a stop bit
 *                        will be sent on the bus after the transaction.
 *                        This is usually required for normal operation. If
 *                        this parameter is zero then no stop bit will
 *                        be omitted. In this case, no other task can use
 *                        the component until a stop bit has been sent.
 *
 * \returns               #I2C_ACK if the write was acknowledged by the device, #I2C_NACK otherwise.
 */
i2c_res_t i2c_master_write(
        i2c_master_t *ctx,
        uint8_t device_addr,
        uint8_t *buf,
        size_t n,
        size_t *num_bytes_sent,
        int send_stop_bit);

/**
 * Reads data from an I2C bus as a master.
 *
 * \param ctx             A pointer to the I2C master context to use.
 * \param device_addr     The address of the device to read from.
 * \param buf             The buffer to fill with data.
 * \param n               The number of bytes to read.
 * \param send_stop_bit   If this is non-zero then a stop bit.
 *                        will be sent on the bus after the transaction.
 *                        This is usually required for normal operation. If
 *                        this parameter is zero then no stop bit will
 *                        be omitted. In this case, no other task can use
 *                        the component until a stop bit has been sent.
 *
 * \returns               #I2C_ACK if the read was acknowledged by the device, #I2C_NACK otherwise.
 */
i2c_res_t i2c_master_read(
        i2c_master_t *ctx,
        uint8_t device_addr,
        uint8_t *buf,
        size_t n,
        int send_stop_bit);

/**
 * Send a stop bit to an I2C bus as a master.
 *
 * This function will cause a stop bit to be sent on the bus. It should
 * be used to complete/abort a transaction if the ``send_stop_bit`` argument
 * was not set when calling the i2c_master_read() or i2c_master_write()
 * functions.
 *
 * \param ctx             A pointer to the I2C master context to use.
 */
void i2c_master_stop_bit_send(
        i2c_master_t *ctx);


/**
 * Implements an I2C master device on one or two single or multi-bit ports.
 *
 * \param ctx                 A pointer to the I2C master context to initialize.
 * \param p_scl               The port containing SCL. This may be either the same as
 *                            or different than \p p_sda.
 * \param scl_bit_position    The bit number of the SCL line on the port \p p_scl.
 * \param scl_other_bits_mask A value that is ORed into the port value driven to \p p_scl
 *                            both when SCL is high and low. The bit representing SCL (as
 *                            well as SDA if they share the same port) must be set to 0.
 * \param p_sda               The port containing SDA. This may be either the same as
 *                            or different than \p p_scl.
 * \param sda_bit_position    The bit number of the SDA line on the port \p p_sda.
 * \param sda_other_bits_mask A value that is ORed into the port value driven to \p p_sda
 *                            both when SDA is high and low. The bit representing SDA (as
 *                            well as SCL if they share the same port) must be set to 0.
 * \param kbits_per_second    The speed of the I2C bus. The maximum value allowed is 400.
 */
void i2c_master_init(
        i2c_master_t *ctx,
        port_t p_scl,
        uint32_t scl_bit_position,
        uint32_t scl_other_bits_mask,
        port_t p_sda,
        uint32_t sda_bit_position,
        uint32_t sda_other_bits_mask,
        unsigned kbits_per_second);


/**
 * Shuts down the I2C master device.
 *
 * \param ctx  A pointer to the I2C master context to shut down.
 *
 * This function disables the ports associated with the I2C master
 * and deallocates its timer if it was not provided by the application.
 *
 * If subsequent reads or writes need to be performed, then i2c_master_init()
 * must be called again first.
 */
void i2c_master_shutdown(i2c_master_t *ctx);

#endif  // !__XC__

#endif
