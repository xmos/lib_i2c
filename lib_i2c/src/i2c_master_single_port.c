// Copyright (c) 2020, XMOS Ltd, All rights reserved
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <xcore/triggerable.h>
#include <xcore/hwtimer.h>
#include <xcore/assert.h>
#include "xclib.h"
#include "i2c_c.h"
#define SDA_LOW     0
#define SCL_LOW     0


#define BIT_TIME(KBITS_PER_SEC) ((XS1_TIMER_MHZ * 1000) / KBITS_PER_SEC)
#define BIT_MASK(BIT_POS) (1 << BIT_POS)

#define FOUR_POINT_SEVEN_MICRO_SECONDS_IN_TICKS     470
#define ONE_POINT_THREE_MICRO_SECONDS_IN_TICKS      130
#define JITTER_TICKS    3
#define WAKEUP_TICKS    10

/** Reads back the SCL line, waiting until it goes high (in
 *  case the slave is clock stretching). It is assumed that the clock
 *  line has been release (driven high) before calling this function.
 *  Since the line going high may be delayed, the fall_time value may
 *  need to be adjusted
 */
static void wait_for_clock_high(i2c_master_sp_ctx_t* ctx,
                                uint32_t* fall_time,
                                uint32_t delay) {
    uint32_t time;
    uint32_t val;
    hwtimer_t tmr = ctx->tmr;

    val = port_peek(ctx->p_i2c);
    while (!(val & ctx->scl_high)) {
        val = port_peek(ctx->p_i2c);
    }

    time = hwtimer_wait_until(tmr, *fall_time + delay);

    // Adjust timing due to support clock stretching without clock drift in the
    // normal case.

    // If the time is beyond the time it takes simply to wake up and start
    // executing then the clock needs to be adjusted
    if (time > *fall_time + delay + WAKEUP_TICKS) {
        *fall_time = time - delay - WAKEUP_TICKS;
    }
}

static void high_pulse_drive(i2c_master_sp_ctx_t* ctx,
                             int sdaValue,
                             uint32_t* fall_time) {
    const uint32_t SCL_HIGH = ctx->scl_high;
    const uint32_t SDA_HIGH = ctx->sda_high;
    hwtimer_t tmr = ctx->tmr;

    sdaValue = sdaValue ? SDA_HIGH : SDA_LOW;

    port_out(ctx->p_i2c, SCL_LOW  | sdaValue | ctx->other_bits_mask);
    (void) hwtimer_wait_until(tmr, *fall_time + ctx->low_period_ticks);
    port_out(ctx->p_i2c, SCL_HIGH  | sdaValue | ctx->other_bits_mask);
    wait_for_clock_high(ctx, fall_time, (ctx->bit_time * 3) / 4);
    *fall_time += ctx->bit_time;
    (void) hwtimer_wait_until(tmr, *fall_time);
    port_out(ctx->p_i2c, SCL_LOW  | sdaValue | ctx->other_bits_mask);
}

static int high_pulse_sample(i2c_master_sp_ctx_t* ctx,
                                         uint32_t* fall_time) {
    const uint32_t SCL_HIGH = ctx->scl_high;
    const uint32_t SDA_HIGH = ctx->sda_high;
    hwtimer_t tmr = ctx->tmr;
    int sample_value;

    port_out(ctx->p_i2c, SCL_LOW  | SDA_HIGH | ctx->other_bits_mask);
    (void) hwtimer_wait_until(tmr, *fall_time + ctx->low_period_ticks);
    port_out(ctx->p_i2c, SCL_HIGH  | SDA_HIGH | ctx->other_bits_mask);
    wait_for_clock_high(ctx, fall_time, (ctx->bit_time * 3) / 4);
    sample_value = (port_peek(ctx->p_i2c) & SDA_HIGH) ? 1 : 0;
    *fall_time += ctx->bit_time;
    (void) hwtimer_wait_until(tmr, *fall_time);
    port_out(ctx->p_i2c, SCL_LOW  | SDA_HIGH | ctx->other_bits_mask);

    return sample_value;
}

static void start_bit(i2c_master_sp_ctx_t* ctx,
                      uint32_t* fall_time) {
    const uint32_t SCL_HIGH = ctx->scl_high;
    const uint32_t SDA_HIGH = ctx->sda_high;
    hwtimer_t tmr = ctx->tmr;
    uint32_t stopped = ctx->stopped;

    if (!stopped) {
        (void) hwtimer_wait_until(tmr, *fall_time + ctx->low_period_ticks);
        port_out(ctx->p_i2c, SCL_HIGH  | SDA_HIGH | ctx->other_bits_mask);
        wait_for_clock_high(ctx, fall_time, ctx->bus_off_ticks);
    }
    port_out(ctx->p_i2c, SCL_HIGH  | SDA_LOW | ctx->other_bits_mask);
    hwtimer_delay(tmr, ctx->bit_time / 2);
    port_out(ctx->p_i2c, SCL_LOW  | SDA_LOW | ctx->other_bits_mask);
    *fall_time = hwtimer_get_time(tmr);
}

static void stop_bit(i2c_master_sp_ctx_t* ctx,
                     uint32_t* fall_time) {
    const uint32_t SCL_HIGH = BIT_MASK(ctx->scl_bit_position);
    const uint32_t SDA_HIGH = BIT_MASK(ctx->sda_bit_position);
    hwtimer_t tmr = ctx->tmr;

    port_out(ctx->p_i2c, SCL_LOW  | SDA_LOW | ctx->other_bits_mask);
    (void) hwtimer_wait_until(tmr, *fall_time + ctx->low_period_ticks);
    port_out(ctx->p_i2c, SCL_HIGH  | SDA_LOW | ctx->other_bits_mask);
    wait_for_clock_high(ctx, fall_time, ctx->bit_time);
    port_out(ctx->p_i2c, SCL_HIGH  | SDA_HIGH | ctx->other_bits_mask);
    hwtimer_delay(tmr, ctx->bus_off_ticks);
}

static int tx8(i2c_master_sp_ctx_t* ctx,
                    uint32_t data,
                    uint32_t* fall_time) {
    uint32_t bit_rev_data = ((uint32_t) bitrev(data)) >> 24;
    for (int i = 8; i != 0; i--) {
        high_pulse_drive(ctx, bit_rev_data & 1, fall_time);
        bit_rev_data >>= 1;
    }
    return high_pulse_sample(ctx, fall_time);
}

i2c_res_t i2c_master_sp_read(i2c_master_sp_ctx_t* ctx,
        uint8_t device,
        uint8_t buf[],
        size_t m,
        int send_stop_bit) {
    const uint32_t SCL_HIGH = ctx->scl_high;
    const uint32_t SDA_HIGH = ctx->sda_high;
    i2c_res_t result;
    port_t p_i2c = ctx->p_i2c;
    hwtimer_t tmr = ctx->tmr;
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

            // ACK after every read byte until the final byte then NACK.
            unsigned sda = SDA_LOW;
            if (j == m-1) {
              sda = SDA_HIGH;
            }

            port_out(p_i2c, SCL_LOW | sda | ctx->other_bits_mask);
            (void) hwtimer_wait_until(tmr, fall_time + ctx->low_period_ticks);
            port_out(p_i2c, SCL_HIGH | sda | ctx->other_bits_mask);
            wait_for_clock_high(ctx, &fall_time, (ctx->bit_time * 3) / 4);
            fall_time += ctx->bit_time;
            (void) hwtimer_wait_until(tmr, fall_time);
            port_out(p_i2c, SCL_LOW | SDA_HIGH | ctx->other_bits_mask);
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

i2c_res_t i2c_master_sp_write(
        i2c_master_sp_ctx_t *ctx,
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
    for (; j < n; j++) {
        if (ack != 0) {
            break;
        }
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

void i2c_master_sp_stop_bit_send(
        i2c_master_sp_ctx_t *ctx) {
    uint32_t fall_time;

    fall_time = ctx->last_fall_time;
    stop_bit(ctx, &fall_time);
    ctx->stopped = 1;

    // Remember the last fall time to ensure the next start bit is valid
    ctx->last_fall_time = fall_time;
}

void i2c_master_sp_init(i2c_master_sp_ctx_t* ctx,
    port_t p_i2c,
    hwtimer_t tmr,
    const unsigned kbits_per_second,
    uint32_t scl_bit_position,
    uint32_t sda_bit_position,
    uint32_t other_bits_mask) {

    ctx->p_i2c = p_i2c;
    ctx->scl_bit_position = scl_bit_position;
    ctx->sda_bit_position = sda_bit_position;
    ctx->other_bits_mask = other_bits_mask;
    ctx->bit_time = BIT_TIME(kbits_per_second);
    ctx->scl_high = BIT_MASK(ctx->scl_bit_position);
    ctx->sda_high = BIT_MASK(ctx->sda_bit_position);

    if (tmr != 0) {
        ctx->tmr = tmr;
    } else {
        ctx->tmr = hwtimer_alloc();
        xassert(ctx->tmr != 0);
    }

    if (kbits_per_second <= 100) {
        ctx->low_period_ticks = FOUR_POINT_SEVEN_MICRO_SECONDS_IN_TICKS;
    } else if (kbits_per_second <= 400) {
        ctx->low_period_ticks = ONE_POINT_THREE_MICRO_SECONDS_IN_TICKS;
    } else {
        xassert(0); /* Fast-mode Plus not implemented */
    }

    // There is some jitter on the falling edges of the clock. In order to ensure
    // that the low period is respected we need to extend the minimum low period.
    ctx->low_period_ticks += JITTER_TICKS;

    ctx->bus_off_ticks = ctx->bit_time/2 + ctx->bit_time/16;

    ctx->stopped = 1;
    ctx->last_fall_time = 0;

    port_enable(p_i2c);

    port_write_control_word(p_i2c, XS1_SETC_DRIVE_PULL_UP);

    port_out(p_i2c, ctx->scl_high | ctx->sda_high | ctx->other_bits_mask);
}
