// Copyright (c) 2020, XMOS Ltd, All rights reserved

#define ORIG 0

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <xcore/_support/xcore_meta_macro.h>
#include <xcore/interrupt_wrappers.h>
#include <xcore/triggerable.h>
#include <xcore/assert.h>

//#include "rtos_printf.h"
#include "i2c_c.h"

enum i2c_async_master_state_t {
    IDLE = 0,
    REPEATED_START_CLOCK_LOW,
    REPEATED_START_WAIT_FOR_CLOCK_HIGH,
    REPEATED_START_HOLD_CLOCK_HIGH,
    START_BIT_0,
    START_BIT_1,
    WRITE_0,
    WRITE_1,
    WRITE_ACK_0,
    WRITE_ACK_1,
    WRITE_ACK_2,
    READ_0,
    READ_1,
    READ_2,
    READ_ACK_0,
    READ_ACK_1,
    STOP_BIT_0,
    STOP_BIT_1,
    STOP_BIT_2,
    STOP_BIT_3,
    STOP_BIT_4,
    DONE_NO_STOP,
};

enum optype_t {
    WRITE = 0,
    READ = 1,
    SEND_STOP_BIT = 2
};

enum ack_t {
    ACKED = 0,
    NACKED = 1,
};

/*  Adjust for time slip.
 *
 *  All timings of the state machine in i2c_master_async_comb are made
 *  w.r.t the measured fall_time of the SCL line.
 *  If the task keeps up then we will get the speed requested. However,
 *  if the task falls behind due to other processing on the core then
 *  this function will adjust both the next event time and the fall time
 *  reference to slip so that subsequent timings are correct.
 */
__attribute__((always_inline))
static inline void adjust_for_slip(
        uint32_t now,
        uint32_t *event_time,
        uint32_t *fall_time)
{
    // This value is the minimum number of timer ticks we estimate we can
    // get to a new event to from now.
    const int SLIP_THRESHOLD = 100;
    if (*event_time - now < SLIP_THRESHOLD) {
//        rtos_printf("s");
        int new_event_time = now + SLIP_THRESHOLD;
        if (fall_time != NULL) {
            *fall_time += new_event_time - *event_time;
        }
        *event_time = new_event_time;
    }
}

__attribute__((always_inline))
static inline int adjust_fall(
        uint32_t event_time,
        uint32_t now,
        uint32_t fall_time)
{
    const int SLIP_THRESHOLD = 100;
    if (now - event_time > SLIP_THRESHOLD) {
        fall_time = fall_time + (now - event_time);
    }
    return fall_time;
}

#if ORIG
DEFINE_INTERRUPT_CALLBACK(i2c_isr_grp, i2c_scl_isr, data)
{
    i2c_master_t *ctx = data;

    uint32_t now;

    (void) port_in(ctx->p_scl);
    now = hwtimer_get_time(ctx->tmr);

    switch (ctx->state) {
    case WRITE_0:
    case WRITE_ACK_0:
    case READ_ACK_0:
    case STOP_BIT_0:
    case REPEATED_START_WAIT_FOR_CLOCK_HIGH:
    case READ_0:
        ctx->fall_time += ctx->bit_time;
        ctx->event_time = ctx->fall_time;
        adjust_for_slip(now, &ctx->event_time, &ctx->fall_time);
        break;
    case WRITE_ACK_1:
        ctx->fall_time += ctx->bit_time;
        ctx->event_time = ctx->fall_time - ctx->bit_time / 4;
        adjust_for_slip(now, &ctx->event_time, &ctx->fall_time);
        ctx->state = WRITE_ACK_2;
        break;
    case STOP_BIT_2:
        ctx->event_time = now + ctx->bit_time / 2;
        adjust_for_slip(now, &ctx->event_time, NULL);
        ctx->state = STOP_BIT_3;
        break;
    case READ_1:
        ctx->fall_time += ctx->bit_time;
        ctx->event_time = ctx->fall_time - ctx->bit_time / 4;
        adjust_for_slip(now, &ctx->event_time, &ctx->fall_time);
        ctx->state = READ_2;
        break;
    default:
        xassert(0);
        break;
    }

    ctx->waiting_for_clock_release = 0;
    triggerable_disable_trigger(ctx->p_scl);

    ctx->timer_enabled = 1;
    hwtimer_set_trigger_time(ctx->tmr, ctx->event_time);
    triggerable_enable_trigger(ctx->tmr);
}

DEFINE_INTERRUPT_CALLBACK(i2c_isr_grp, i2c_timer_isr, data)
{
    i2c_master_t *ctx = data;

    uint32_t now;
    uint32_t event_time;

    now = hwtimer_get_time(ctx->tmr);
    event_time = ctx->event_time;

//    uint32_t diff = now - event_time;
//    if (diff > 150) {
//        rtos_printf("%d\n", diff);
//    }

    switch (ctx->state) {
    case REPEATED_START_CLOCK_LOW:
        // The operation is finished, but no stop bit is being written
        port_out(ctx->p_scl, 0);
        event_time = now + ctx->bit_time / 2;
        ctx->state = REPEATED_START_WAIT_FOR_CLOCK_HIGH;
        break;
    case REPEATED_START_WAIT_FOR_CLOCK_HIGH:
        (void) port_in(ctx->p_scl);
        ctx->timer_enabled = 0;
        ctx->waiting_for_clock_release = 1;
        ctx->state = REPEATED_START_HOLD_CLOCK_HIGH;
        break;
    case REPEATED_START_HOLD_CLOCK_HIGH:
        event_time = now + ctx->bit_time / 2;
        ctx->state = START_BIT_0;
        break;
    case START_BIT_0:
        port_out(ctx->p_sda, 0);
        event_time = now + ctx->bit_time / 2;
        ctx->state = START_BIT_1;
        break;
    case START_BIT_1:
        ctx->fall_time = now;
        // Fallthrough to WRITE_0 state
    case WRITE_0:
        port_out(ctx->p_scl, 0);
        ctx->fall_time = adjust_fall(event_time, now, ctx->fall_time);
        port_out(ctx->p_sda, ctx->data >> 7);
        ctx->data <<= 1;
        ctx->bitnum++;
        ctx->state = WRITE_1;
        event_time = ctx->fall_time + ctx->bit_time / 2 + ctx->bit_time / 32;
        adjust_for_slip(now, &event_time, &ctx->fall_time);
        break;
    case WRITE_1:
        (void) port_in(ctx->p_scl);
        ctx->timer_enabled = 0;
        ctx->waiting_for_clock_release = 1;
        if (ctx->bitnum == 8) {
            ctx->state = WRITE_ACK_0;
        } else {
            ctx->state = WRITE_0;
        }
        break;
    case WRITE_ACK_0:
        port_out(ctx->p_scl, 0);
        (void) port_in(ctx->p_sda);
        ctx->fall_time = adjust_fall(event_time, now, ctx->fall_time);
        event_time = ctx->fall_time + ctx->bit_time / 2 + ctx->bit_time / 32;
        adjust_for_slip(now, &event_time, &ctx->fall_time);
        ctx->state = WRITE_ACK_1;
        break;
    case WRITE_ACK_1:
        (void) port_in(ctx->p_scl);
        ctx->timer_enabled = 0;
        ctx->waiting_for_clock_release = 1;
        break;
    case WRITE_ACK_2: {
        int ack;
        ack = port_in(ctx->p_sda);
        event_time = ctx->fall_time;
        adjust_for_slip(now, &event_time, &ctx->fall_time);
        if (ack == ACKED && ctx->optype == WRITE) {
            ctx->bytes_sent++;
            const int all_data_sent = (ctx->bytes_sent == ctx->num_bytes);
            if (all_data_sent) {
                // The master and slave disagree since the slave should nack
                // the last byte.
                ctx->res = I2C_ACK;
                if (ctx->send_stop_bit) {
                    ctx->state = STOP_BIT_0;
                } else {
                    ctx->state = DONE_NO_STOP;
                }
            } else {
                // get next byte of data.
                ctx->data = ctx->buf[ctx->bytes_sent];
                ctx->bitnum = 0;
                // Now go back to the transmitting
                ctx->state = WRITE_0;
            }
        } else if (ack == NACKED && ctx->optype == WRITE) {
            ctx->bytes_sent++;
            const int all_data_sent = (ctx->bytes_sent == ctx->num_bytes);
            if (all_data_sent) {
                // The master and slave agree that this is the end of the operation.
                ctx->res = I2C_NACK;
                if (ctx->send_stop_bit) {
                    ctx->state = STOP_BIT_0;
                } else {
                    ctx->state = DONE_NO_STOP;
                }
            } else {
                // The slave has aborted the operation.
                ctx->res = I2C_NACK;
                if (ctx->send_stop_bit) {
                    ctx->state = STOP_BIT_0;
                } else {
                    ctx->state = DONE_NO_STOP;
                }
            }
        } else if (ack == ACKED && ctx->optype == READ) {
            // The slave has acked the addr, we can go ahead with the operation.
            ctx->data = 0;
            ctx->bitnum = 0;
            ctx->bytes_sent++;
            ctx->state = READ_0;
            ctx->res = I2C_ACK;
        } else if (ack == NACKED && ctx->optype == READ) {
            // The slave has nacked the addr (or the slave isn't there). Abort.
            ctx->res = I2C_NACK;
            if (ctx->send_stop_bit) {
                ctx->state = STOP_BIT_0;
            } else {
                ctx->state = DONE_NO_STOP;
            }
        }
        break;
    }
    case READ_0:
        port_out(ctx->p_scl, 0);
        (void) port_in(ctx->p_sda);
        ctx->fall_time = adjust_fall(event_time, now, ctx->fall_time);
        ctx->bitnum++;
        ctx->state = READ_1;
        event_time = ctx->fall_time + ctx->bit_time / 2 + ctx->bit_time / 32;
        adjust_for_slip(now, &event_time, &ctx->fall_time);
        break;
    case READ_1:
        (void) port_in(ctx->p_scl);
        ctx->timer_enabled = 0;
        ctx->waiting_for_clock_release = 1;
        break;
    case READ_2: {
        int bit;
        bit = port_in(ctx->p_sda);
        ctx->data <<= 1;
        ctx->data += bit & 1;
        event_time = ctx->fall_time;
        adjust_for_slip(now, &event_time, &ctx->fall_time);
        if (ctx->bitnum == 8) {
            ctx->buf[ctx->bytes_sent] = ctx->data;
            ctx->bytes_sent++;
            ctx->state = READ_ACK_0;
        } else {
            ctx->state = READ_0;
        }
        break;
    }
    case READ_ACK_0:
        port_out(ctx->p_scl, 0);
        if (ctx->bytes_sent == ctx->num_bytes) {
            (void) port_in(ctx->p_sda);
        } else {
            port_out(ctx->p_sda, 0);
        }
        ctx->fall_time = adjust_fall(event_time, now, ctx->fall_time);
        ctx->state = READ_ACK_1;
        event_time = ctx->fall_time + ctx->bit_time / 2 + ctx->bit_time / 32;
        adjust_for_slip(now, &event_time, &ctx->fall_time);
        break;
    case READ_ACK_1:
        (void) port_in(ctx->p_scl);
        ctx->timer_enabled = 0;
        ctx->waiting_for_clock_release = 1;
        if (ctx->bytes_sent == ctx->num_bytes) {
            if (ctx->send_stop_bit) {
                ctx->state = STOP_BIT_0;
            } else {
                ctx->state = DONE_NO_STOP;
            }
        } else {
            ctx->data = 0;
            ctx->bitnum = 0;
            ctx->state = READ_0;
        }
        break;
    case STOP_BIT_0:
        port_out(ctx->p_scl, 0);
        ctx->fall_time = adjust_fall(event_time, now, ctx->fall_time);
        event_time = ctx->fall_time + ctx->bit_time / 4;
        adjust_for_slip(now, &event_time, &ctx->fall_time);
        ctx->state = STOP_BIT_1;
        break;
    case STOP_BIT_1:
        port_out(ctx->p_sda, 0);
        event_time = ctx->fall_time + ctx->bit_time / 2;
        adjust_for_slip(now, &event_time, &ctx->fall_time);
        ctx->state = STOP_BIT_2;
        break;
    case STOP_BIT_2:
        (void) port_in(ctx->p_scl);
        ctx->timer_enabled = 0;
        ctx->waiting_for_clock_release = 1;
        break;
    case STOP_BIT_3:
        (void) port_in(ctx->p_sda);
        event_time = now + ctx->bit_time / 4;
        ctx->state = STOP_BIT_4;

        // Know that the next transaction can start from the stopped state
        ctx->stopped = 1;
        break;
    case DONE_NO_STOP:
        // Know that the next transaction needs to create a repeated start
        ctx->stopped = 0;
        // Fallthrough to STOP_BIT_4 code
    case STOP_BIT_4:
        ctx->timer_enabled = 0;
        ctx->state = IDLE;
        break;
    default:
        xassert(0);
        break;
    }

    if (ctx->timer_enabled) {
        if (event_time - ctx->event_time < 125) {
            rtos_printf("%d: %d\n", ctx->state, event_time - ctx->event_time);
        }

        hwtimer_change_trigger_time(ctx->tmr, event_time);
        ctx->event_time = event_time;
    } else {
        triggerable_disable_trigger(ctx->tmr);
    }

    if (ctx->waiting_for_clock_release) {
        port_set_trigger_in_equal(ctx->p_scl, 1);
        triggerable_enable_trigger(ctx->p_scl);
    }

    if (ctx->state == IDLE) {
        if (ctx->operation_complete != NULL) {
            ctx->operation_complete(ctx);
        }
    }
}
#else

__attribute__((always_inline))
static inline uint32_t scl_isr(i2c_master_t *ctx) {
    uint32_t now;

    (void) port_in(ctx->p_scl);
    now = hwtimer_get_time(ctx->tmr);

    switch (ctx->state) {
    case WRITE_0:
    case WRITE_ACK_0:
    case READ_ACK_0:
    case STOP_BIT_0:
    case REPEATED_START_HOLD_CLOCK_HIGH:
    case READ_0:
        ctx->fall_time += ctx->bit_time;
        ctx->event_time = ctx->fall_time;
//        adjust_for_slip(now, &ctx->event_time, &ctx->fall_time);
        break;
    case WRITE_ACK_1:
        ctx->fall_time += ctx->bit_time;
        ctx->event_time = ctx->fall_time - ctx->bit_time / 4;
//        adjust_for_slip(now, &ctx->event_time, &ctx->fall_time);
        ctx->state = WRITE_ACK_2;
        break;
    case STOP_BIT_2:
        ctx->event_time = now + ctx->bit_time / 2;
//        adjust_for_slip(now, &ctx->event_time, NULL);
        ctx->state = STOP_BIT_3;
        break;
    case READ_1:
        ctx->fall_time += ctx->bit_time;
        ctx->event_time = ctx->fall_time - ctx->bit_time / 4;
//        adjust_for_slip(now, &ctx->event_time, &ctx->fall_time);
        ctx->state = READ_2;
        break;
    default:
        xassert(0);
        break;
    }

    ctx->waiting_for_clock_release = 0;
    port_clear_trigger_in(ctx->p_scl);
    triggerable_disable_trigger(ctx->p_scl);

    ctx->timer_enabled = 1;
    hwtimer_set_trigger_time(ctx->tmr, ctx->event_time);
    triggerable_enable_trigger(ctx->tmr);

    return now;
}

__attribute__((always_inline))
static inline uint32_t timer_isr(i2c_master_t *ctx) {
    uint32_t now;
    uint32_t event_time;

    now = hwtimer_get_time(ctx->tmr);
    event_time = ctx->event_time;

//    uint32_t diff = now - event_time;
//    if (diff > 150) {
//        rtos_printf("%d\n", diff);
//    }

    switch (ctx->state) {
    case REPEATED_START_CLOCK_LOW:
        // The operation is finished, but no stop bit is being written
        port_out(ctx->p_scl, 0);
        event_time = now + ctx->bit_time / 2 + ctx->bit_time / 32;
        port_set_trigger_time(ctx->p_scl, port_get_trigger_time(ctx->p_scl) + ctx->bit_time / 2 + ctx->bit_time / 32);
        ctx->state = REPEATED_START_WAIT_FOR_CLOCK_HIGH;
        break;
    case REPEATED_START_WAIT_FOR_CLOCK_HIGH:
        (void) port_in(ctx->p_scl);
        ctx->timer_enabled = 0;
        ctx->waiting_for_clock_release = 1;
        ctx->state = REPEATED_START_HOLD_CLOCK_HIGH;
        break;
    case REPEATED_START_HOLD_CLOCK_HIGH:
        event_time = now + ctx->bit_time / 2;
        ctx->state = START_BIT_0;
        break;
    case START_BIT_0:
        port_out(ctx->p_sda, 0);
        event_time = now + ctx->bit_time / 2;
        ctx->state = START_BIT_1;
        break;
    case START_BIT_1:
        ctx->fall_time = now;
        // Fallthrough to WRITE_0 state
    case WRITE_0:
        port_out(ctx->p_scl, 0);
        ctx->fall_time = adjust_fall(event_time, now, ctx->fall_time);
        port_out(ctx->p_sda, ctx->data >> 7);
        ctx->data <<= 1;
        ctx->bitnum++;
        ctx->state = WRITE_1;
        //event_time = ctx->fall_time + ctx->bit_time / 2 + ctx->bit_time / 32;
        event_time = now + ctx->bit_time / 2 + ctx->bit_time / 32;
        port_set_trigger_time(ctx->p_scl, port_get_trigger_time(ctx->p_scl) + ctx->bit_time / 2 + ctx->bit_time / 32);
//        rtos_printf("%d -> %d (%d)\n", now, event_time, event_time-now);
//        adjust_for_slip(now, &event_time, &ctx->fall_time);
        break;
    case WRITE_1:
        (void) port_in(ctx->p_scl);
        ctx->timer_enabled = 0;
        ctx->waiting_for_clock_release = 1;
        if (ctx->bitnum == 8) {
            ctx->state = WRITE_ACK_0;
        } else {
            ctx->state = WRITE_0;
        }
        break;
    case WRITE_ACK_0:
        port_out(ctx->p_scl, 0);
        (void) port_in(ctx->p_sda);
        ctx->fall_time = adjust_fall(event_time, now, ctx->fall_time);
        //event_time = ctx->fall_time + ctx->bit_time / 2 + ctx->bit_time / 32;
        event_time = now + ctx->bit_time / 2 + ctx->bit_time / 32;
        port_set_trigger_time(ctx->p_scl, port_get_trigger_time(ctx->p_scl) + ctx->bit_time / 2 + ctx->bit_time / 32);
//        rtos_printf("%d -> %d (%d)\n", now, event_time, event_time-now);
//        adjust_for_slip(now, &event_time, &ctx->fall_time);
        ctx->state = WRITE_ACK_1;
        break;
    case WRITE_ACK_1:
        (void) port_in(ctx->p_scl);
        ctx->timer_enabled = 0;
        ctx->waiting_for_clock_release = 1;
        break;
    case WRITE_ACK_2: {
        int ack;
        ack = port_in(ctx->p_sda);
        event_time = ctx->fall_time;
//        adjust_for_slip(now, &event_time, &ctx->fall_time);
        if (ack == ACKED && ctx->optype == WRITE) {
            ctx->bytes_sent++;
            const int all_data_sent = (ctx->bytes_sent == ctx->num_bytes);
            if (all_data_sent) {
                // The master and slave disagree since the slave should nack
                // the last byte.
                ctx->res = I2C_ACK;
                if (ctx->send_stop_bit) {
                    ctx->state = STOP_BIT_0;
                } else {
                    ctx->state = DONE_NO_STOP;
                }
            } else {
                // get next byte of data.
                ctx->data = ctx->buf[ctx->bytes_sent];
                ctx->bitnum = 0;
                // Now go back to the transmitting
                ctx->state = WRITE_0;
            }
        } else if (ack == NACKED && ctx->optype == WRITE) {
            ctx->bytes_sent++;
            const int all_data_sent = (ctx->bytes_sent == ctx->num_bytes);
            if (all_data_sent) {
                // The master and slave agree that this is the end of the operation.
                ctx->res = I2C_NACK;
                if (ctx->send_stop_bit) {
                    ctx->state = STOP_BIT_0;
                } else {
                    ctx->state = DONE_NO_STOP;
                }
            } else {
                // The slave has aborted the operation.
                ctx->res = I2C_NACK;
                if (ctx->send_stop_bit) {
                    ctx->state = STOP_BIT_0;
                } else {
                    ctx->state = DONE_NO_STOP;
                }
            }
        } else if (ack == ACKED && ctx->optype == READ) {
            // The slave has acked the addr, we can go ahead with the operation.
            ctx->data = 0;
            ctx->bitnum = 0;
            ctx->bytes_sent++;
            ctx->state = READ_0;
            ctx->res = I2C_ACK;
        } else if (ack == NACKED && ctx->optype == READ) {
            // The slave has nacked the addr (or the slave isn't there). Abort.
            ctx->res = I2C_NACK;
            if (ctx->send_stop_bit) {
                ctx->state = STOP_BIT_0;
            } else {
                ctx->state = DONE_NO_STOP;
            }
        }
        break;
    }
    case READ_0:
        port_out(ctx->p_scl, 0);
        (void) port_in(ctx->p_sda);
        ctx->fall_time = adjust_fall(event_time, now, ctx->fall_time);
        ctx->bitnum++;
        ctx->state = READ_1;
        //event_time = ctx->fall_time + ctx->bit_time / 2 + ctx->bit_time / 32;
        event_time = now + ctx->bit_time / 2 + ctx->bit_time / 32;
        port_set_trigger_time(ctx->p_scl, port_get_trigger_time(ctx->p_scl) + ctx->bit_time / 2 + ctx->bit_time / 32);
//        rtos_printf("%d -> %d (%d)\n", now, event_time, event_time-now);
//        adjust_for_slip(now, &event_time, &ctx->fall_time);
        break;
    case READ_1:
        (void) port_in(ctx->p_scl);
        ctx->timer_enabled = 0;
        ctx->waiting_for_clock_release = 1;
        break;
    case READ_2: {
        int bit;
        bit = port_in(ctx->p_sda);
        ctx->data <<= 1;
        ctx->data += bit & 1;
        event_time = ctx->fall_time;
//        adjust_for_slip(now, &event_time, &ctx->fall_time);
        if (ctx->bitnum == 8) {
            ctx->buf[ctx->bytes_sent] = ctx->data;
            ctx->bytes_sent++;
            ctx->state = READ_ACK_0;
        } else {
            ctx->state = READ_0;
        }
        break;
    }
    case READ_ACK_0:
        port_out(ctx->p_scl, 0);
        if (ctx->bytes_sent == ctx->num_bytes) {
            (void) port_in(ctx->p_sda);
        } else {
            port_out(ctx->p_sda, 0);
        }
        ctx->fall_time = adjust_fall(event_time, now, ctx->fall_time);
        ctx->state = READ_ACK_1;
        //event_time = ctx->fall_time + ctx->bit_time / 2 + ctx->bit_time / 32;
        event_time = now + ctx->bit_time / 2 + ctx->bit_time / 32;
//        rtos_printf("%d -> %d (%d)\n", now, event_time, event_time-now);
//        adjust_for_slip(now, &event_time, &ctx->fall_time);
        break;
    case READ_ACK_1:
        (void) port_in(ctx->p_scl);
        ctx->timer_enabled = 0;
        ctx->waiting_for_clock_release = 1;
        if (ctx->bytes_sent == ctx->num_bytes) {
            if (ctx->send_stop_bit) {
                ctx->state = STOP_BIT_0;
            } else {
                ctx->state = DONE_NO_STOP;
            }
        } else {
            ctx->data = 0;
            ctx->bitnum = 0;
            ctx->state = READ_0;
        }
        break;
    case STOP_BIT_0:
        port_out(ctx->p_scl, 0);
        //ctx->fall_time = adjust_fall(event_time, now, ctx->fall_time);
        ctx->fall_time = now;
        event_time = ctx->fall_time + ctx->bit_time / 4;
//        adjust_for_slip(now, &event_time, &ctx->fall_time);
        ctx->state = STOP_BIT_1;
        break;
    case STOP_BIT_1:
        port_out(ctx->p_sda, 0);
        event_time = ctx->fall_time + ctx->bit_time / 2 + ctx->bit_time / 32;
        port_set_trigger_time(ctx->p_scl, port_get_trigger_time(ctx->p_scl) + ctx->bit_time / 2 + ctx->bit_time / 32);
//        adjust_for_slip(now, &event_time, &ctx->fall_time);
        ctx->state = STOP_BIT_2;
        break;
    case STOP_BIT_2:
        (void) port_in(ctx->p_scl);
        ctx->timer_enabled = 0;
        ctx->waiting_for_clock_release = 1;
        break;
    case STOP_BIT_3:
        (void) port_in(ctx->p_sda);
        event_time = now + ctx->bit_time / 4;
        ctx->state = STOP_BIT_4;

        // Know that the next transaction can start from the stopped state
        ctx->stopped = 1;
        break;
    case DONE_NO_STOP:
        // Know that the next transaction needs to create a repeated start
        ctx->stopped = 0;
        // Fallthrough to STOP_BIT_4 code
    case STOP_BIT_4:
        ctx->timer_enabled = 0;
        ctx->state = IDLE;
        break;
    default:
        xassert(0);
        break;
    }

    if (ctx->timer_enabled) {
//        if (event_time - ctx->event_time < 125) {
//            rtos_printf("%d: %d\n", ctx->state, event_time - ctx->event_time);
//        }

        hwtimer_change_trigger_time(ctx->tmr, event_time);
        ctx->event_time = event_time;
    } else {
        hwtimer_clear_trigger_time(ctx->tmr);
        triggerable_disable_trigger(ctx->tmr);
    }

    if (ctx->waiting_for_clock_release) {
        port_set_trigger_in_equal(ctx->p_scl, 1);
        triggerable_enable_trigger(ctx->p_scl);
    }

    if (ctx->state == IDLE) {
        if (ctx->operation_complete != NULL) {
            ctx->operation_complete(ctx);
        }
    }

    return now;
}

DEFINE_INTERRUPT_CALLBACK(i2c_isr_grp, i2c_isr, data)
{
    i2c_master_t *ctx = data;
    uint32_t now;

    while (ctx->waiting_for_clock_release || ctx->timer_enabled) {
        if (ctx->waiting_for_clock_release) {
            now = scl_isr(ctx);
        } else if (ctx->timer_enabled) {
            now = timer_isr(ctx);
        }

//        if (ctx->timer_enabled) {
//            //adjust_for_slip(now, &ctx->event_time, &ctx->fall_time);
//            if (ctx->event_time - now >= 100) {
//                //break;
//            } else {
//                //rtos_printf("%d ", ctx->event_time - now);
//                //xassert(0);
//                //rtos_printf("%d ", ctx->event_time - now);
//            }
//        } else if (ctx->waiting_for_clock_release) {
//            //break;
//        } else {
//            break;
//        }
    }
}
#endif

static void i2c_start_transaction(
        i2c_master_t *ctx,
        uint8_t device_addr,
        uint8_t *buf,
        size_t n,
        int send_stop_bit)
{
    ctx->buf = buf;
    ctx->data |= (device_addr << 1);
    ctx->bitnum = 0;
    ctx->num_bytes = n;
    ctx->send_stop_bit = send_stop_bit;
    // The 'bytes_sent' variable gets increment after every byte *including*
    // the addr bytes. So we set it to -1 to be 0 after the addr byte.
    ctx->bytes_sent = -1;

    if (!ctx->stopped) {
        ctx->state = REPEATED_START_CLOCK_LOW;
    } else {
        ctx->state = START_BIT_0;
    }

    ///NOT REALLY NEEDED///
    ctx->waiting_for_clock_release = 0;
    port_clear_trigger_in(ctx->p_scl);
    triggerable_disable_trigger(ctx->p_scl);
    ///////////////////////

    ctx->timer_enabled = 1;
    ctx->event_time = hwtimer_get_time(ctx->tmr);
    hwtimer_set_trigger_time(ctx->tmr, ctx->event_time);
    triggerable_enable_trigger(ctx->tmr);
}

i2c_res_t i2c_master_write(
        i2c_master_t *ctx,
        uint8_t device_addr,
        uint8_t *buf,
        size_t n,
        int send_stop_bit)
{
    if (ctx->state == IDLE) {
        ctx->data = 0;
        ctx->optype = WRITE;

        i2c_start_transaction(ctx, device_addr, buf, n, send_stop_bit);

        return I2C_STARTED;
    } else {
        return I2C_NOT_STARTED;
    }
}

i2c_res_t i2c_master_read(
        i2c_master_t *ctx,
        uint8_t device_addr,
        uint8_t *buf,
        size_t n,
        int send_stop_bit)
{
    if (ctx->state == IDLE) {
        ctx->data = 1;
        ctx->optype = READ;

        i2c_start_transaction(ctx, device_addr, buf, n, send_stop_bit);

        return I2C_STARTED;
    } else {
        return I2C_NOT_STARTED;
    }
}

i2c_res_t i2c_result_get(
        i2c_master_t *ctx,
        ssize_t *num_bytes_transferred)
{
    if (num_bytes_transferred != NULL) {
        *num_bytes_transferred = ctx->bytes_sent;
    }

    return ctx->res;
}

i2c_res_t i2c_master_stop_bit_send(
        i2c_master_t *ctx)
{
    if (ctx->state == IDLE) {
        ctx->state = STOP_BIT_0;
        ctx->timer_enabled = 1;
        ctx->event_time = hwtimer_get_time(ctx->tmr);
        hwtimer_set_trigger_time(ctx->tmr, ctx->event_time);
        triggerable_enable_trigger(ctx->tmr);
        return I2C_STARTED;
    } else {
        return I2C_NOT_STARTED;
    }
}

#define BIT_TIME(KBITS_PER_SEC) ((XS1_TIMER_MHZ * 1000) / KBITS_PER_SEC)

void i2c_master_init(
        i2c_master_t *ctx,
        port_t p_scl,
        port_t p_sda,
        const unsigned kbits_per_second,
        const size_t max_transaction_size,
        void *app_data,
        i2c_operation_complete_t op_complete)
{
    memset(ctx, 0, sizeof(i2c_master_t));

    port_enable(p_scl);
    port_enable(p_sda);

    ctx->tmr = hwtimer_alloc();
    xassert(ctx->tmr != 0);

    ctx->p_scl = p_scl;
    ctx->p_sda = p_sda;

    ctx->app_data = app_data;
    ctx->operation_complete = op_complete;
    ctx->max_transaction_size = max_transaction_size;
    ctx->kbits_per_second = kbits_per_second;
    ctx->bit_time = BIT_TIME(kbits_per_second);

    ctx->stopped = 1;
    ctx->res = I2C_ACK;

#if ORIG
    triggerable_setup_interrupt_callback(ctx->tmr,   ctx, INTERRUPT_CALLBACK(i2c_timer_isr));
    triggerable_setup_interrupt_callback(ctx->p_scl, ctx, INTERRUPT_CALLBACK(i2c_scl_isr));
#else
    triggerable_setup_interrupt_callback(ctx->tmr,   ctx, INTERRUPT_CALLBACK(i2c_isr));
    triggerable_setup_interrupt_callback(ctx->p_scl, ctx, INTERRUPT_CALLBACK(i2c_isr));
#endif
}
