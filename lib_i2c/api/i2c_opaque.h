#ifndef _i2c_master_opaque_h_
#define _i2c_master_opaque_h_

#include <i2c_common.h>

#ifndef __XC__
#include <xcore/port.h>
#endif

//typedef unsigned i2c_master_opaque_t;

#ifdef __XC__
#define UNSAFE unsafe
#else
#define UNSAFE
#endif

void i2c_master_opaque_init(intptr_t ctx_opaque,
        port_t p_scl,
        uint32_t scl_bit_position,
        uint32_t scl_other_bits_mask,
        port_t p_sda,
        uint32_t sda_bit_position,
        uint32_t sda_other_bits_mask,
        unsigned kbits_per_second);

i2c_res_t i2c_master_opaque_read(
        intptr_t ctx_opaque,
        uint8_t device_addr,
        uint8_t * UNSAFE buf,
        size_t n,
        int send_stop_bit);

i2c_res_t i2c_master_opaque_write(
        intptr_t ctx_opaque,
        uint8_t device_addr,
        uint8_t * UNSAFE buf,
        size_t n,
        size_t * UNSAFE num_bytes_sent,
        int send_stop_bit);

void i2c_master_opaque_stop_bit_send(intptr_t ctx_opaque);

void i2c_master_opaque_shutdown(intptr_t ctx_opaque);

#endif
