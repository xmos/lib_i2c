#ifndef _i2c_common_h_
#define _i2c_common_h_

#ifdef __XC__
typedef port port_t;
#endif


/**
 * Status codes for I2C master operations
 */
typedef enum {
  I2C_NACK,        /**< The slave has NACKed the last byte. */
  I2C_ACK,         /**< The slave has ACKed the last byte. */
  I2C_STARTED,     /**< The requested I2C transaction has started. */
  I2C_NOT_STARTED  /**< The requested I2C transaction could not start. */
} i2c_res_t;


/** This type is used by the supplementary I2C register read/write functions to
 *  report back on whether the operation was a success or not.
 */
typedef enum {
  I2C_REGOP_SUCCESS,     ///< the operation was successful
  I2C_REGOP_DEVICE_NACK, ///< the operation was NACKed when sending the device address, so either the device is missing or busy
  I2C_REGOP_INCOMPLETE   ///< the operation was NACKed halfway through by the slave
} i2c_regop_res_t;

#endif
