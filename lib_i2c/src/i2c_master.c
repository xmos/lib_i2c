// Copyright (c) 2011-2020, XMOS Ltd, All rights reserved
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <xclib.h>
#include <xcore/triggerable.h>
#include <xcore/hwtimer.h>
#include <xcore/assert.h>

#include "i2c_c.h"

#define SDA_LOW     0
#define SCL_LOW     0

#define BIT_TIME(KBITS_PER_SEC) ((XS1_TIMER_MHZ * 1000) / KBITS_PER_SEC)
#define BIT_MASK(BIT_POS) (1 << BIT_POS)

#define FOUR_POINT_SEVEN_MICRO_SECONDS_IN_TICKS     470
#define ONE_POINT_THREE_MICRO_SECONDS_IN_TICKS      130
#define JITTER_TICKS    3
#define WAKEUP_TICKS    10

/** Return the number of 10ns timer ticks required to meet the timing as defined
 *  in the standards.
 */
__attribute__((always_inline))
static inline uint32_t compute_low_period_ticks(
        const unsigned kbits_per_second)
{
    uint32_t ticks = 0;

    if (kbits_per_second <= 100) {
        ticks = FOUR_POINT_SEVEN_MICRO_SECONDS_IN_TICKS;
    } else if (kbits_per_second <= 400) {
        ticks = ONE_POINT_THREE_MICRO_SECONDS_IN_TICKS;
    } else {
        /* "Fast-mode Plus not implemented" */xassert(0);
    }

    // There is some jitter on the falling edges of the clock. In order to ensure
    // that the low period is respected we need to extend the minimum low period.
    return ticks + JITTER_TICKS;
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

/** Reads back the SCL line, waiting until it goes high (in
 *  case the slave is clock stretching). It is assumed that the clock
 *  line has been release (driven high) before calling this function.
 *  Since the line going high may be delayed, the fall_time value may
 *  need to be adjusted
 */
static void wait_for_clock_high(
        i2c_master_t* ctx,
        uint32_t* fall_time,
        uint32_t delay) {
    const uint32_t SCL_HIGH = ctx->scl_high;
    const port_t p_scl = ctx->p_scl;
    uint32_t now;
    uint32_t val;
    hwtimer_t tmr = ctx->tmr;

    asm volatile ("nop" ::);
    val = port_peek(p_scl);
    while (!(val & SCL_HIGH)) {
        val = port_peek(p_scl);
    }

    now = hwtimer_wait_until(tmr, *fall_time + delay);

    // Adjust timing due to support clock stretching without clock drift in the
    // normal case.

    // If the time is beyond the time it takes simply to wake up and start
    // executing then the clock needs to be adjusted
    if (now > *fall_time + delay + WAKEUP_TICKS) {
        *fall_time = now - delay - WAKEUP_TICKS;
    }
}

static void high_pulse_drive(
        i2c_master_t* ctx,
        int sdaValue,
        uint32_t* fall_time) {
    const uint32_t SCL_HIGH = ctx->scl_high;
    const uint32_t SDA_HIGH = ctx->sda_high;
    const port_t p_sda = ctx->p_sda;
    const port_t p_scl = ctx->p_scl;
    const uint32_t bit_time = ctx->bit_time;
    const uint32_t three_quarter_bit_time = ctx->three_quarter_bit_time;
    const uint32_t low_period_ticks = ctx->low_period_ticks;
    const uint32_t bus_off_ticks = ctx->bus_off_ticks;
    const uint32_t scl_other_bits_mask = ctx->scl_other_bits_mask;
    const uint32_t sda_other_bits_mask = ctx->sda_other_bits_mask;
    hwtimer_t tmr = ctx->tmr;

    sdaValue = sdaValue ? SDA_HIGH : SDA_LOW;

    if (p_scl == p_sda) {
        port_out(p_scl, SCL_LOW  | sdaValue | scl_other_bits_mask);
        (void) hwtimer_wait_until(tmr, *fall_time + low_period_ticks);
        port_out(p_scl, SCL_HIGH | sdaValue | scl_other_bits_mask);
        wait_for_clock_high(ctx, fall_time, three_quarter_bit_time);
        *fall_time += bit_time;
        (void) hwtimer_wait_until(tmr, *fall_time);
        port_out(p_scl, SCL_LOW  | sdaValue | scl_other_bits_mask);
    } else {
        port_out(p_scl, SCL_LOW  | scl_other_bits_mask);
        port_out(p_sda, sdaValue | sda_other_bits_mask);
        (void) hwtimer_wait_until(tmr, *fall_time + low_period_ticks);
        port_out(p_scl, SCL_HIGH | scl_other_bits_mask);
        wait_for_clock_high(ctx, fall_time, three_quarter_bit_time);
        *fall_time += bit_time;
        (void) hwtimer_wait_until(tmr, *fall_time);
        port_out(p_scl, SCL_LOW  | scl_other_bits_mask);
    }
}

/** 'Pulse' the clock line high and in the middle of the high period
 *  sample the data line (if required). Timing is done via the fall_time
 *  reference and bit_time period supplied.
 */
__attribute__((always_inline))
static inline int high_pulse_sample(
        const i2c_master_t *ctx,
        uint32_t *fall_time) {
    const uint32_t SCL_HIGH = ctx->scl_high;
    const uint32_t SDA_HIGH = ctx->sda_high;
    const port_t p_sda = ctx->p_sda;
    const port_t p_scl = ctx->p_scl;
    const hwtimer_t tmr = ctx->tmr;
    const uint32_t bit_time = ctx->bit_time;
    const uint32_t three_quarter_bit_time = ctx->three_quarter_bit_time;
    const uint32_t low_period_ticks = ctx->low_period_ticks;
    const uint32_t bus_off_ticks = ctx->bus_off_ticks;
    const uint32_t scl_other_bits_mask = ctx->scl_other_bits_mask;
    const uint32_t sda_other_bits_mask = ctx->sda_other_bits_mask;

    uint32_t sample_value = 0;

    if (p_scl == p_sda) {
        port_out(p_scl, SCL_LOW  | SDA_HIGH | scl_other_bits_mask);
        (void) hwtimer_wait_until(tmr, *fall_time + low_period_ticks);
        port_out(p_scl, SCL_HIGH | SDA_HIGH | scl_other_bits_mask);
        wait_for_clock_high(ctx, fall_time, three_quarter_bit_time);
        sample_value = (port_peek(p_scl) & SDA_HIGH) ? 1 : 0;
        *fall_time += bit_time;
        (void) hwtimer_wait_until(tmr, *fall_time);
        port_out(p_scl, SCL_LOW  | SDA_HIGH | scl_other_bits_mask);
    } else {
        port_out(p_scl, SCL_LOW  | scl_other_bits_mask);
        port_out(p_sda, SDA_HIGH | sda_other_bits_mask);
        (void) hwtimer_wait_until(tmr, *fall_time + low_period_ticks);
        port_out(p_scl, SCL_HIGH | scl_other_bits_mask);
        wait_for_clock_high(ctx, fall_time, three_quarter_bit_time);
        sample_value = (port_peek(p_sda) & SDA_HIGH) ? 1 : 0;
        *fall_time += bit_time;
        (void) hwtimer_wait_until(tmr, *fall_time);
        port_out(p_scl, SCL_LOW  | scl_other_bits_mask);
    }

    return sample_value;
}

/** Output a start bit. The function returns the 'fall time' i.e. the
 *  reference clock time when the SCL line transitions to low.
 */
static void start_bit(
        const i2c_master_t *ctx,
        uint32_t *fall_time) {
    const uint32_t SCL_HIGH = ctx->scl_high;
    const uint32_t SDA_HIGH = ctx->sda_high;
    const port_t p_sda = ctx->p_sda;
    const port_t p_scl = ctx->p_scl;
    const hwtimer_t tmr = ctx->tmr;
    const uint32_t half_bit_time = ctx->half_bit_time;
    const uint32_t low_period_ticks = ctx->low_period_ticks;
    const uint32_t bus_off_ticks = ctx->bus_off_ticks;
    const uint32_t sda_other_bits_mask = ctx->sda_other_bits_mask;
    const uint32_t scl_other_bits_mask = ctx->scl_other_bits_mask;

    if (!ctx->stopped) {
        (void) hwtimer_wait_until(tmr, *fall_time + low_period_ticks);
        if (p_scl == p_sda) {
            port_out(p_scl, SCL_HIGH | SDA_HIGH | scl_other_bits_mask);
        } else {
            port_out(p_scl, SCL_HIGH | scl_other_bits_mask);
            port_out(p_sda, SDA_HIGH | sda_other_bits_mask);
        }
        wait_for_clock_high(ctx, fall_time, bus_off_ticks);
    }

    if (p_scl == p_sda) {
        port_out(p_scl, SCL_HIGH | SDA_LOW | scl_other_bits_mask);
        hwtimer_delay(tmr, half_bit_time);
        port_out(p_scl, SCL_LOW  | SDA_LOW | scl_other_bits_mask);
    } else {
        port_out(p_scl, SCL_HIGH | scl_other_bits_mask);
        port_out(p_sda, SDA_LOW  | sda_other_bits_mask);
        hwtimer_delay(tmr, half_bit_time);
        port_out(p_scl, SCL_LOW  | scl_other_bits_mask);
    }
    *fall_time = hwtimer_get_time(tmr);
}

/** Output a stop bit.
 */
static void stop_bit(
        const i2c_master_t *ctx,
        uint32_t *fall_time) {
    const uint32_t SCL_HIGH = ctx->scl_high;
    const uint32_t SDA_HIGH = ctx->sda_high;
    const port_t p_scl = ctx->p_scl;
    const port_t p_sda = ctx->p_sda;
    const hwtimer_t tmr = ctx->tmr;
    const uint32_t bit_time = ctx->bit_time;
    const uint32_t low_period_ticks = ctx->low_period_ticks;
    const uint32_t bus_off_ticks = ctx->bus_off_ticks;
    const uint32_t sda_other_bits_mask = ctx->sda_other_bits_mask;
    const uint32_t scl_other_bits_mask = ctx->scl_other_bits_mask;

    if (p_scl == p_sda) {
        port_out(p_scl, SCL_LOW  | SDA_LOW  | scl_other_bits_mask);
        (void) hwtimer_wait_until(tmr, *fall_time + low_period_ticks);
        port_out(p_scl, SCL_HIGH | SDA_LOW  | scl_other_bits_mask);
        wait_for_clock_high(ctx, fall_time, bit_time);
        port_out(p_scl, SCL_HIGH | SDA_HIGH | scl_other_bits_mask);
        hwtimer_delay(tmr, bus_off_ticks);
    } else {
        port_out(p_scl, SCL_LOW  | scl_other_bits_mask);
        port_out(p_sda, SDA_LOW  | sda_other_bits_mask);
        (void) hwtimer_wait_until(tmr, *fall_time + low_period_ticks);
        port_out(p_scl, SCL_HIGH | scl_other_bits_mask);
        // port_out(p_sda, SDA_LOW  | sda_other_bits_mask);
        wait_for_clock_high(ctx, fall_time, bit_time);
        port_out(p_scl, SCL_HIGH | scl_other_bits_mask);
        port_out(p_sda, SDA_HIGH | sda_other_bits_mask);
        hwtimer_delay(tmr, bus_off_ticks);
    }
}

/** Transmit 8 bits of data, then read the ack back from the slave and return
 *  that value.
 */
static int tx8(
        const i2c_master_t *ctx,
        uint32_t data,
        uint32_t *fall_time) {
    // Data is transmitted MSB first
    data = bitrev(data) >> 24;
    for (int i = 8; i != 0; i--) {
        high_pulse_drive(ctx, data & 1, fall_time);
        data >>= 1;
    }
    return high_pulse_sample(ctx, fall_time);
}

i2c_res_t i2c_master_read(
        i2c_master_t *ctx,
        uint8_t device,
        uint8_t buf[],
        size_t m,
        int send_stop_bit) {
    i2c_res_t result;
    const uint32_t SCL_HIGH = ctx->scl_high;
    const uint32_t SDA_HIGH = ctx->sda_high;
    const port_t p_scl = ctx->p_scl;
    const port_t p_sda = ctx->p_sda;
    const hwtimer_t tmr = ctx->tmr;
    const uint32_t bit_time = ctx->bit_time;
    const uint32_t quarter_bit_time = ctx->quarter_bit_time;
    const uint32_t low_period_ticks = ctx->low_period_ticks;
    const uint32_t three_quarter_bit_time = ctx->three_quarter_bit_time;
    const uint32_t sda_other_bits_mask = ctx->sda_other_bits_mask;
    const uint32_t scl_other_bits_mask = ctx->scl_other_bits_mask;
    uint32_t fall_time = ctx->last_fall_time;

    start_bit(ctx, &fall_time);

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

            uint32_t sda = SDA_LOW;
            if (j == m-1) {
              sda = SDA_HIGH;
            }

            if (p_scl == p_sda) {
                port_out(p_scl, SCL_LOW  | sda | scl_other_bits_mask);
                (void) hwtimer_wait_until(tmr, fall_time + low_period_ticks);
                port_out(p_scl, SCL_HIGH | sda | scl_other_bits_mask);
                wait_for_clock_high(ctx, &fall_time, three_quarter_bit_time);
                fall_time += bit_time;
                (void) hwtimer_wait_until(tmr, fall_time);
                port_out(p_scl, SCL_LOW  | SDA_HIGH | scl_other_bits_mask);
            } else {
                port_out(p_sda, sda      | sda_other_bits_mask);
                port_out(p_scl, SCL_LOW  | scl_other_bits_mask);
                (void) hwtimer_wait_until(tmr, fall_time + low_period_ticks);
                port_out(p_scl, SCL_HIGH | scl_other_bits_mask);
                wait_for_clock_high(ctx, &fall_time, three_quarter_bit_time);
                fall_time += bit_time;
                (void) hwtimer_wait_until(tmr, fall_time);
                port_out(p_sda, SDA_HIGH | sda_other_bits_mask);
                port_out(p_scl, SCL_LOW  | scl_other_bits_mask);
            }
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
        int send_stop_bit) {
    i2c_res_t result;
    uint32_t fall_time = ctx->last_fall_time;

    start_bit(ctx, &fall_time);
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

    if (num_bytes_sent != NULL) {
        *num_bytes_sent = j;
    }

    result = (ack == 0) ? I2C_ACK : I2C_NACK;

    // Remember the last fall time to ensure the next start bit is valid
    ctx->last_fall_time = fall_time;

    return result;
}

void i2c_master_stop_bit_send(
        i2c_master_t *ctx) {
    uint32_t fall_time = ctx->last_fall_time;
    stop_bit(ctx, &fall_time);
    ctx->stopped = 1;

    // Remember the last fall time to ensure the next start bit is valid
    ctx->last_fall_time = fall_time;
}

void i2c_master_init(
        i2c_master_t *ctx,
        const port_t p_scl,
        const uint32_t scl_bit_position,
        const uint32_t scl_other_bits_mask,
        const port_t p_sda,
        const uint32_t sda_bit_position,
        const uint32_t sda_other_bits_mask,
        hwtimer_t tmr,
        const unsigned kbits_per_second) {
    memset(ctx, 0, sizeof(i2c_master_t));

    if (tmr != 0) {
        ctx->tmr = tmr;
    } else {
        ctx->tmr = hwtimer_alloc();
        xassert(ctx->tmr != 0);
    }

    ctx->p_scl = p_scl;
    ctx->p_sda = p_sda;
    ctx->scl_bit_position = scl_bit_position;
    ctx->scl_other_bits_mask = scl_other_bits_mask;
    ctx->sda_bit_position = sda_bit_position;
    ctx->sda_other_bits_mask = sda_other_bits_mask;

    ctx->scl_high = BIT_MASK(ctx->scl_bit_position);
    ctx->sda_high = BIT_MASK(ctx->sda_bit_position);

    ctx->kbits_per_second = kbits_per_second;
    ctx->bit_time = BIT_TIME(kbits_per_second);
    ctx->three_quarter_bit_time = (ctx->bit_time * 3) / 4;
    ctx->half_bit_time = ctx->bit_time / 2;
    ctx->quarter_bit_time = ctx->bit_time / 4;
    ctx->low_period_ticks = compute_low_period_ticks(kbits_per_second);
    ctx->bus_off_ticks = compute_bus_off_ticks(ctx->bit_time);

    ctx->stopped = 1;
    ctx->last_fall_time = 0;

    port_enable(p_scl);
    port_write_control_word(p_scl, XS1_SETC_DRIVE_PULL_UP);

    if (p_scl == p_sda) {
        port_out(p_scl, ctx->scl_high | ctx->sda_high | ctx->scl_other_bits_mask);
    } else {
        port_enable(p_sda);
        port_write_control_word(p_sda, XS1_SETC_DRIVE_PULL_UP); // todo only if multibit
    }
}

void i2c_master_shutdown(
        i2c_master_t *ctx) {
    if (ctx->tmr != 0) {
        hwtimer_free(ctx->tmr);
    }
}
