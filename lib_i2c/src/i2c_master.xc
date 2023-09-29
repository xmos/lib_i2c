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

            // TODO: lbuf needs to be statically sized, but how to handle longer arrays?
            uint8_t lbuf[8];
            unsafe {
                i2c_master_t * unsafe pctx = &ctx;
                uint8_t * unsafe plbuf = &lbuf[0];
                result = i2c_master_read(pctx, p_scl, p_sda, device, plbuf, m, send_stop_bit);
            }
            for (int j = 0; j < m; ++j) {
                buf[j] = lbuf[j];
            }
            locked_client = -1;
            break;

        case (size_t i = 0; i < n; ++i)
            (locked_client == -1 || i == locked_client) =>
            c[i].write(uint8_t device, uint8_t buf[m], size_t m, size_t &num_bytes_sent, int send_stop_bit) -> i2c_res_t result:

            uint8_t lbuf[8];
            for (int j = 0; j < m; ++j) {
                lbuf[j] = buf[j];
            }
            size_t bytes = num_bytes_sent;
            unsafe {
                i2c_master_t * unsafe pctx = &ctx;
                uint8_t * unsafe plbuf = &lbuf[0];
                size_t * unsafe pbytes = &bytes;
                result = i2c_master_write(pctx, p_scl, p_sda, device, plbuf, m, pbytes, send_stop_bit);
            }
            num_bytes_sent = bytes;
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
