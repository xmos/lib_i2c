// Copyright (c) 2020, XMOS Ltd, All rights reserved
#ifndef _i2c_c_h_
#define _i2c_c_h_

#include <sys/types.h>
#include <xcore/port.h>
#include <xcore/clock.h>
#include <xcore/hwtimer.h>

typedef enum {
  I2C_NACK,        ///< the slave has NACKed the last byte
  I2C_ACK,         ///< the slave has ACKed the last byte
  I2C_STARTED,     ///< the requested I2C transaction has started
  I2C_NOT_STARTED  ///< the requested I2C transaction could not start
} i2c_res_t;

/** This type is used by the supplementary I2C register read/write functions to
 *  report back on whether the operation was a success or not.
 */
typedef enum {
  I2C_REGOP_SUCCESS,     ///< the operation was successful
  I2C_REGOP_DEVICE_NACK, ///< the operation was NACKed when sending the device address, so either the device is missing or busy
  I2C_REGOP_INCOMPLETE,  ///< the operation was NACKed halfway through by the slave
  I2C_REGOP_STARTED,     ///< the operation was started
  I2C_REGOP_NOT_STARTED  ///< the operation could not start
} i2c_regop_res_t;



typedef struct i2c_master_struct i2c_master_t;

struct i2c_master_struct {
    port_t p_scl;
    port_t p_sda;
    hwtimer_t tmr;
    unsigned kbits_per_second;

    uint32_t bit_time;
    uint32_t quarter_bit_time;
    uint32_t half_bit_time;
    uint32_t three_quarter_bit_time;
    uint32_t low_period_ticks;
    uint32_t bus_off_ticks;

    uint32_t last_fall_time;
    int stopped;
};

i2c_res_t i2c_master_write(
        i2c_master_t *ctx,
        uint8_t device,
        uint8_t buf[],
        size_t n,
        size_t *num_bytes_sent,
        int send_stop_bit);

i2c_res_t i2c_master_read(
        i2c_master_t *ctx,
        uint8_t device,
        uint8_t buf[],
        size_t m,
        int send_stop_bit);

void i2c_master_stop_bit_send(
        i2c_master_t *ctx);

void i2c_master_init(
        i2c_master_t *ctx,
        port_t p_scl,
        port_t p_sda,
        hwtimer_t tmr,
        const unsigned kbits_per_second);


#define I2C_CALLBACK_ATTR __attribute__((fptrgroup("i2c_callback")))

typedef struct i2c_master_async_struct i2c_master_async_t;

typedef void (*i2c_master_async_operation_complete_t)(i2c_master_async_t *ctx);

struct i2c_master_async_struct {
    port_t p_scl;
    port_t p_sda;
    unsigned kbits_per_second;
    uint32_t bit_time; /*= BIT_TIME(kbits_per_second);*/
    uint32_t half_bit_time;
    uint32_t quarter_bit_time;
    uint32_t scl_low_time;

    uint8_t *buf; /* buffer provided by app */
    hwtimer_t tmr; /* not the shared one, but could be assigned the shared timer? */
    int state; /* init to IDLE */

    int waiting_for_clock_release;
    int timer_enabled;

    int optype;
    uint32_t fall_time;
    int data;
    int bitnum;
    int bytes_sent;
    int num_bytes;
    uint32_t event_time;
    int send_stop_bit;
    int stopped; /* init to 1 */
    i2c_res_t res; /* init to I2C_ACK? */

    I2C_CALLBACK_ATTR i2c_master_async_operation_complete_t operation_complete;
    void *app_data;
};

i2c_res_t i2c_master_async_write(
        i2c_master_async_t *ctx,
        uint8_t device_addr,
        uint8_t *buf,
        size_t n,
        int send_stop_bit);

i2c_res_t i2c_master_async_read(
        i2c_master_async_t *ctx,
        uint8_t device_addr,
        uint8_t *buf,
        size_t n,
        int send_stop_bit);

i2c_res_t i2c_master_async_stop_bit_send(
        i2c_master_async_t *ctx);

i2c_res_t i2c_master_async_result_get(
        i2c_master_async_t *ctx,
        ssize_t *num_bytes_transferred);

void i2c_master_async_init(
        i2c_master_async_t *ctx,
        port_t p_scl,
        port_t p_sda,
        const unsigned kbits_per_second,
        void *app_data,
        i2c_master_async_operation_complete_t op_complete);


/** I2C Slave Response
*
*  This type is used to describe the I2C slave response.
*/
typedef enum i2c_slave_ack_t {
    I2C_SLAVE_ACK,  ///<< ACK to accept request
    I2C_SLAVE_NACK, ///<< NACK to ignore request
} i2c_slave_ack_t;

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
typedef i2c_slave_ack_t (*ack_read_request_t)(void *app_data);

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
typedef i2c_slave_ack_t (*ack_write_request_t)(void *app_data);

/** Master requires data.
 *
 *  This callback function will be called when the I2C master requires data
 *  from the slave.
 *
 *  \return   the data to pass to the master.
 */
typedef uint8_t (*master_requires_data_t)(void *app_data);

/** Master has sent some data.
 *
 *  This callback function will be called when the I2C master has transferred
 *  a byte of data to the slave.
 */
typedef i2c_slave_ack_t (*master_sent_data_t)(void *app_data, uint8_t data);

/** Stop bit.
*
*  This callback function will be called by the component when a stop bit
*  is sent by the master.
*/
typedef void (*stop_bit_t)(void *app_data);

/** Shutdown the I2C component.
 *
 *  This function will cause the I2C slave task to shutdown and return.
 */
typedef int (*shutdown_t)(void *app_data);

/**
 * Callback group representing callback events that can occur during the
 * operation of the sI2C lave task. Must be initialized by the application
 * prior to passing it to one of the I2C tasks.
 */
typedef struct {
    I2C_CALLBACK_ATTR ack_read_request_t ack_read_request;
    I2C_CALLBACK_ATTR ack_write_request_t ack_write_request;
    I2C_CALLBACK_ATTR master_requires_data_t master_requires_data;
    I2C_CALLBACK_ATTR master_sent_data_t master_sent_data;
    I2C_CALLBACK_ATTR stop_bit_t stop_bit;
    I2C_CALLBACK_ATTR shutdown_t shutdown;
    void *app_data;
} i2c_callback_group_t;

/** I2C slave task.
 *
 *  This function instantiates an i2c_slave component.
 *
 *  \param i2c_cbg     The I2C callback group pointing to the application's
 *                     functions to use for initialization and getting and
 *                     receiving frames. Also points to application specific
 *                     data which will be shared between the callbacks.
 *  \param  p_scl      the SCL port of the I2C bus
 *  \param  p_sda      the SDA port of the I2C bus
 *  \param device_addr the address of the slave device
 *
 */
void i2c_slave(const i2c_callback_group_t *const i2c_cbg,
               port_t p_scl,
               port_t p_sda,
               uint8_t device_addr);


typedef struct i2c_master_sp_ctx {
    port_t p_i2c;
    uint32_t scl_bit_position;
    uint32_t sda_bit_position;
    uint32_t other_bits_mask;

    uint32_t scl_high;
    uint32_t sda_high;

    hwtimer_t tmr;
    uint32_t bit_time;
    uint32_t low_period_ticks;
    uint32_t bus_off_ticks;

    uint32_t last_fall_time;
    uint32_t stopped;
} i2c_master_sp_ctx_t;

void i2c_master_sp_init(i2c_master_sp_ctx_t* ctx,
        port_t p_i2c,
        hwtimer_t tmr,
        const unsigned kbits_per_second,
        uint32_t scl_bit_position,
        uint32_t sda_bit_position,
        uint32_t other_bits_mask);

i2c_res_t i2c_master_sp_read(i2c_master_sp_ctx_t* ctx,
        uint8_t device,
        uint8_t buf[],
        size_t m,
        int send_stop_bit);

i2c_res_t i2c_master_sp_write(
        i2c_master_sp_ctx_t *ctx,
        uint8_t device,
        uint8_t buf[],
        size_t n,
        size_t *num_bytes_sent,
        int send_stop_bit);

void i2c_master_sp_stop_bit_send(
        i2c_master_sp_ctx_t *ctx);

#include "i2c_c_reg.h"

#endif
