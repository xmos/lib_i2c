#include <i2c_master_if.h>
#include <i2c_opaque.h>
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
    uint8_t ctx[sizeof(i2c_master_t)];
    intptr_t ctx_opaque = (intptr_t)&ctx[0];
    i2c_master_opaque_init(ctx_opaque, p_scl, 0, 0, p_sda, 0, 0, kbits_per_second);

    int locked_client = -1;

    while (1) {
        select {
        case (size_t i = 0; i < n; ++i)
            (locked_client == -1 || i == locked_client) =>
            c[i].read(uint8_t device, uint8_t buf[m], size_t m, int send_stop_bit) -> i2c_res_t result:

            // TODO: lbuf needs to be statically sized, but how to handle longer arrays?
            uint8_t lbuf[8];
            unsafe {
                uint8_t * unsafe plbuf = &lbuf[0];
                result = i2c_master_opaque_read(ctx_opaque, device, plbuf, m, send_stop_bit);
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
                uint8_t * unsafe plbuf = &lbuf[0];
                size_t * unsafe pbytes = &bytes;
                result = i2c_master_opaque_write(ctx_opaque, device, plbuf, m, pbytes, send_stop_bit);
            }
            num_bytes_sent = bytes;
            locked_client = -1;
            break;

        case c[int i].send_stop_bit(void):
            i2c_master_opaque_stop_bit_send(ctx_opaque);
            locked_client = -1;
            break;

        case c[int i].shutdown(void):
            i2c_master_opaque_shutdown(ctx_opaque);
            return;
        }
    }
}
