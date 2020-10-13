// Copyright (c) 2011-2020, XMOS Ltd, All rights reserved

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <xclib.h>
//#include <xcore/_support/xcore_meta_macro.h>
//#include <xcore/interrupt_wrappers.h>
//#include <xcore/triggerable.h>
#include <xcore/assert.h>

#include "i2c_c.h"

#define BIT_TIME(KBITS_PER_SEC) ((XS1_TIMER_MHZ * 1000) / KBITS_PER_SEC)

/* NOTE: the kbits_per_second needs to be passed around due to the fact that the
 *       compiler won't compute a new static const from a static const.
 */

/** Return the number of 10ns timer ticks required to meet the timing as defined
 *  in the standards.
 */
__attribute__((always_inline))
static inline uint32_t compute_low_period_ticks(
        const unsigned kbits_per_second)
{
    uint32_t ticks = 0;

    if (kbits_per_second <= 100) {
        const uint32_t four_point_seven_micro_seconds_in_ticks = 470;
        ticks = four_point_seven_micro_seconds_in_ticks;
    } else if (kbits_per_second <= 400) {
        const uint32_t one_point_three_micro_seconds_in_ticks = 130;
        ticks = one_point_three_micro_seconds_in_ticks;
    } else {
        /* "Fast-mode Plus not implemented" */xassert(0);
    }

    // There is some jitter on the falling edges of the clock. In order to ensure
    // that the low period is respected we need to extend the minimum low period.
    const uint32_t jitter_ticks = 3;

    return ticks + jitter_ticks;
}

__attribute__((always_inline))
static inline uint32_t compute_bus_off_ticks(
        const uint32_t bit_time)
{
    // Ensure the bus off time is respected. This is just over 1/2 bit time in
    // the case of the Fast-mode I2C so adding bit_time/16 ensures the timing
    // will be enforced
    return bit_time / 2 + bit_time / 16;
}

/** Releases the SCL line, reads it back and waits until it goes high (in
 *  case the slave is clock stretching).
 *  Since the line going high may be delayed, the fall_time value may
 *  need to be adjusted
 */
static void release_clock_and_wait(
        const i2c_master_t *ctx,
        uint32_t *fall_time,
        uint32_t delay)
{
    const port_t p_scl = ctx->p_scl;
    const hwtimer_t tmr = ctx->tmr;

    uint32_t wait_until = *fall_time + delay;

    (void) port_in_when_pinseq(p_scl, PORT_UNBUFFERED, 1);

    uint32_t now;
    now = hwtimer_wait_until(tmr, wait_until);

    // Adjust timing due to support clock stretching without clock drift in the
    // normal case.

    // If the time is beyond the time it takes simply to wake up and start
    // executing then the clock needs to be adjusted
    const int wake_up_ticks = 10;
    if (now > wait_until + wake_up_ticks) {
        *fall_time = now - delay - wake_up_ticks;
    }
}

/** 'Pulse' the clock line high and in the middle of the high period
 *  sample the data line (if required). Timing is done via the fall_time
 *  reference and bit_time period supplied.
 */
__attribute__((always_inline))
static inline int high_pulse_sample(
        const i2c_master_t *ctx,
        uint32_t *fall_time)
{
    const port_t p_sda = ctx->p_sda;
    const port_t p_scl = ctx->p_scl;
    const hwtimer_t tmr = ctx->tmr;
    const uint32_t bit_time = ctx->bit_time;
    const uint32_t three_quarter_bit_time = ctx->three_quarter_bit_time;
    const uint32_t low_period_ticks = ctx->low_period_ticks;

    uint32_t sample_value = 0;

    (void) port_in(p_sda);
    hwtimer_wait_until(tmr, *fall_time + low_period_ticks);
    release_clock_and_wait(ctx, fall_time, three_quarter_bit_time);
    sample_value = port_in(p_sda);

    *fall_time += bit_time;
    hwtimer_wait_until(tmr, *fall_time);
    port_out(p_scl, 0);

    // Mask off all but lowest bit of input - allows use of bit[0] of multibit port
    return sample_value & 1;
}

/** 'Pulse' the clock line high. Timing is done via the fall_time
 *  reference and bit_time period supplied.
 */
__attribute__((always_inline))
static inline void high_pulse(
        const i2c_master_t *ctx,
        uint32_t *fall_time)
{
    const port_t p_scl = ctx->p_scl;
    const hwtimer_t tmr = ctx->tmr;
    const uint32_t bit_time = ctx->bit_time;
    const uint32_t three_quarter_bit_time = ctx->three_quarter_bit_time;
    const uint32_t low_period_ticks = ctx->low_period_ticks;

    hwtimer_wait_until(tmr, *fall_time + low_period_ticks);
    release_clock_and_wait(ctx, fall_time, three_quarter_bit_time);

    *fall_time += bit_time;
    hwtimer_wait_until(tmr, *fall_time);
    port_out(p_scl, 0);
}

/** Output a start bit. The function returns the 'fall time' i.e. the
 *  reference clock time when the SCL line transitions to low.
 */
static void start_bit(
        const i2c_master_t *ctx,
        uint32_t *fall_time,
        int stopped)
{
    const port_t p_sda = ctx->p_sda;
    const port_t p_scl = ctx->p_scl;
    const hwtimer_t tmr = ctx->tmr;
    const uint32_t half_bit_time = ctx->half_bit_time;
    const uint32_t low_period_ticks = ctx->low_period_ticks;
    const uint32_t bus_off_ticks = ctx->bus_off_ticks;

    if (!stopped) {
        hwtimer_wait_until(tmr, *fall_time + low_period_ticks);
        release_clock_and_wait(ctx, fall_time, bus_off_ticks);
    }

    // Drive SDA low
    port_out(p_sda, 0);
    hwtimer_delay(tmr, half_bit_time);
    // Drive SCL low
    port_out(p_scl, 0);

    // Record
    *fall_time = hwtimer_get_time(tmr);
}

/** Output a stop bit.
 */
static void stop_bit(
        const i2c_master_t *ctx,
        uint32_t *fall_time)
{
    const port_t p_sda = ctx->p_sda;
    const hwtimer_t tmr = ctx->tmr;
    const uint32_t bit_time = ctx->bit_time;
    const uint32_t low_period_ticks = ctx->low_period_ticks;
    const uint32_t bus_off_ticks = ctx->bus_off_ticks;

    port_out(p_sda, 0);
    hwtimer_wait_until(tmr, *fall_time + low_period_ticks);
    release_clock_and_wait(ctx, fall_time, bit_time);

    (void) port_in(p_sda);
    hwtimer_delay(tmr, bus_off_ticks);
}

/** Transmit 8 bits of data, then read the ack back from the slave and return
 *  that value.
 */
static int tx8(
        const i2c_master_t *ctx,
        uint32_t data,
        uint32_t *fall_time)
{
    const port_t p_sda = ctx->p_sda;

    // Data is transmitted MSB first
    data = bitrev(data) >> 24;
    for (int i = 8; i != 0; i--) {
        port_out(p_sda, data & 0x1);
        data >>= 1;
        high_pulse(ctx, fall_time);
    }
    return high_pulse_sample(ctx, fall_time);
}

i2c_res_t i2c_master_read(
        i2c_master_t *ctx,
        uint8_t device,
        uint8_t buf[],
        size_t m,
        int send_stop_bit)
{
    i2c_res_t result;

    const port_t p_scl = ctx->p_scl;
    const port_t p_sda = ctx->p_sda;
    const hwtimer_t tmr = ctx->tmr;
    const uint32_t quarter_bit_time = ctx->quarter_bit_time;
    const uint32_t low_period_ticks = ctx->low_period_ticks;
    uint32_t fall_time = ctx->last_fall_time;

    start_bit(ctx, &fall_time, ctx->stopped);

    int ack = tx8(ctx, (device << 1) | 1, &fall_time);
    result = (ack == 0) ? I2C_ACK : I2C_NACK;

    if (result == I2C_ACK) {
        for (int j = 0; j < m; j++) {
            unsigned char data = 0;
            for (int i = 8; i != 0; i--) {
                int temp = high_pulse_sample(ctx, &fall_time);
                data = (data << 1) | temp;
            }
            buf[j] = data;

            hwtimer_wait_until(tmr, fall_time + quarter_bit_time);

            // ACK after every read byte until the final byte then NACK.
            if (j == m - 1) {
                (void) port_in(p_sda);
            } else {
                port_out(p_sda, 0);
            }

            // High pulse but make sure SDA is not driving before lowering SCL
            hwtimer_wait_until(tmr, fall_time + low_period_ticks);
            high_pulse(ctx, &fall_time);
            (void) port_in(p_sda);
        }
    }

    if (send_stop_bit) {
        stop_bit(ctx, &fall_time);
        ctx->stopped = 1;
    } else {
        ctx->stopped = 0;
    }

    // Remember the last fall time to ensure the next start bit is valid
    ctx->last_fall_time = fall_time;

    return result;
}

i2c_res_t i2c_master_write(
        i2c_master_t *ctx,
        uint8_t device,
        uint8_t buf[],
        size_t n,
        size_t *num_bytes_sent,
        int send_stop_bit)
{
    i2c_res_t result;

    const port_t p_scl = ctx->p_scl;
    const port_t p_sda = ctx->p_sda;
    uint32_t fall_time = ctx->last_fall_time;

    start_bit(ctx, &fall_time, ctx->stopped);
    int ack = tx8(ctx, (device << 1) | 0, &fall_time);

    int j = 0;
    for (; j < n && ack == 0; j++) {
        ack = tx8(ctx, buf[j], &fall_time);
    }

    if (send_stop_bit) {
        stop_bit(ctx, &fall_time);
        ctx->stopped = 1;
    } else {
        ctx->stopped = 0;
    }

    *num_bytes_sent = j;

    result = (ack == 0) ? I2C_ACK : I2C_NACK;

    // Remember the last fall time to ensure the next start bit is valid
    ctx->last_fall_time = fall_time;

    return result;
}

void i2c_master_stop_bit_send(
        i2c_master_t *ctx)
{
    uint32_t fall_time = ctx->last_fall_time;
    stop_bit(ctx, &fall_time);
    ctx->stopped = 1;

    // Remember the last fall time to ensure the next start bit is valid
    ctx->last_fall_time = fall_time;
}

void i2c_master_init(
        i2c_master_t *ctx,
        port_t p_scl,
        port_t p_sda,
        hwtimer_t tmr,
        const unsigned kbits_per_second)
{
    memset(ctx, 0, sizeof(i2c_master_t));

    port_enable(p_scl);
    port_enable(p_sda);

    if (tmr != 0) {
        ctx->tmr = tmr;
    } else {
        ctx->tmr = hwtimer_alloc();
        xassert(ctx->tmr != 0);
    }

    ctx->p_scl = p_scl;
    ctx->p_sda = p_sda;

    ctx->kbits_per_second = kbits_per_second;
    ctx->bit_time = BIT_TIME(kbits_per_second);
    ctx->three_quarter_bit_time = (ctx->bit_time * 3) / 4;
    ctx->half_bit_time = ctx->bit_time / 2;
    ctx->quarter_bit_time = ctx->bit_time / 4;
    ctx->low_period_ticks = compute_low_period_ticks(kbits_per_second);
    ctx->bus_off_ticks = compute_bus_off_ticks(ctx->bit_time);

    ctx->stopped = 1;
    ctx->last_fall_time = 0;
}
