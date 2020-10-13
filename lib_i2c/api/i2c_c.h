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

#define I2C_CALLBACK_ATTR __attribute__((fptrgroup("i2c_callback")))

typedef struct i2c_master_async_struct i2c_master_async_t;

typedef void (*i2c_master_async_operation_complete_t)(i2c_master_async_t *ctx);

struct i2c_master_async_struct {
    port_t p_scl;
    port_t p_sda;
    size_t max_transaction_size; /* size of buf */
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
        const size_t max_transaction_size,
        void *app_data,
        i2c_master_async_operation_complete_t op_complete);

#endif
