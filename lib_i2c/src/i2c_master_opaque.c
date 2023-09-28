#include <i2c_opaque.h>
#include <i2c.h>

void i2c_master_opaque_init(intptr_t ctx_opaque,
                port_t p_scl,
                uint32_t scl_bit_position,
                uint32_t scl_other_bits_mask,
                port_t p_sda,
                uint32_t sda_bit_position,
                uint32_t sda_other_bits_mask,
                unsigned kbits_per_second)
{
    i2c_master_t *ctx = (i2c_master_t *)ctx_opaque;
    i2c_master_init(ctx, p_scl, scl_bit_position, scl_other_bits_mask,
                    p_sda, sda_bit_position, sda_other_bits_mask,
                    kbits_per_second);
}

i2c_res_t i2c_master_opaque_read(
                 intptr_t ctx_opaque,
                 uint8_t device_addr,
                 uint8_t *buf,
                 size_t n,
                 int send_stop_bit)
{
    i2c_master_t *ctx = (i2c_master_t *)ctx_opaque;
    return i2c_master_read(ctx, device_addr, buf, n, send_stop_bit);
}

i2c_res_t i2c_master_opaque_write(
                intptr_t ctx_opaque,
                uint8_t device_addr,
                uint8_t *buf,
                size_t n,
                size_t * compat_unsafe num_bytes_sent,
                int send_stop_bit)
{
    i2c_master_t *ctx = (i2c_master_t *)ctx_opaque;
    return i2c_master_write(ctx, device_addr, buf, n, num_bytes_sent, send_stop_bit);
}

void i2c_master_opaque_stop_bit_send(intptr_t ctx_opaque)
{
    i2c_master_t *ctx = (i2c_master_t *)ctx_opaque;
    i2c_master_stop_bit_send(ctx);
}

void i2c_master_opaque_shutdown(intptr_t ctx_opaque)
{
    i2c_master_t *ctx = (i2c_master_t *)ctx_opaque;
    i2c_master_shutdown(ctx);
}
