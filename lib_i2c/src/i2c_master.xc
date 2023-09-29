#include <i2c_master_if.h>
#include <i2c_master.h>

#include <xs1.h>
#include <xccompat.h>


[[distributable]]
void i2c_master(
    server interface i2c_master_if c[n],
    size_t n,
    port p_scl,
    port p_sda,
    static const unsigned kbits_per_second)
{
    i2c_master_t ctx;
    unsafe {
        i2c_master_t * unsafe pctx = &ctx;
        i2c_master_init(pctx, p_scl, 0, 0, p_sda, 0, 0, kbits_per_second);
    }

    int locked_client = -1;

    while (1) {
        select {
        case (size_t i = 0; i < n; ++i)
            (locked_client == -1 || i == locked_client) =>
            c[i].read(uint8_t device, uint8_t buf[m], size_t m, int send_stop_bit) -> i2c_res_t result:

            unsafe {
                i2c_master_t * unsafe pctx = &ctx;
                result = i2c_master_pre_read(pctx, p_scl, p_sda, device);
                if (result == I2C_ACK) {
                    for (int j = 0; j < m; ++j) {
                        buf[j] = i2c_master_read_byte(pctx, p_scl, p_sda, (j == m - 1));
                    }
                }
                i2c_master_post_read(pctx, p_scl, p_sda, send_stop_bit);
            }
            locked_client = -1;
            break;

        case (size_t i = 0; i < n; ++i)
            (locked_client == -1 || i == locked_client) =>
            c[i].write(uint8_t device, uint8_t buf[m], size_t m, size_t &num_bytes_sent, int send_stop_bit) -> i2c_res_t result:

            unsafe {
                i2c_master_t * unsafe pctx = &ctx;
                uint32_t ack = i2c_master_pre_write(pctx, p_scl, p_sda, device);
                size_t j = 0;
                for (; j < m && ack == 0; ++j) {
                    ack = i2c_master_write_byte(pctx, p_scl, p_sda, buf[j]);
                }
                i2c_master_post_write(pctx, p_scl, p_sda, send_stop_bit);
                result = (ack == 0) ? I2C_ACK : I2C_NACK;
                num_bytes_sent = j;
            }
            locked_client = -1;
            break;

        case c[int i].send_stop_bit(void):
            unsafe {
                i2c_master_t * unsafe pctx = &ctx;
                i2c_master_stop_bit_send(pctx, p_scl, p_sda);
            }
            locked_client = -1;
            break;

        case c[int i].shutdown(void):
            unsafe {
                i2c_master_t * unsafe pctx = &ctx;
                i2c_master_shutdown(pctx, p_scl, p_sda);
            }
            return;
        }
    }
}
