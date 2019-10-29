I2C library change log
======================

6.0.0
-----

  * CHANGED: Build files updated to support new "xcommon" behaviour in xwaf.

5.0.1
-----

  * CHANGE: Renamed example application directories to have standard "app"
    prefix.

5.0.0
-----

  * CHANGE: i2c_master_single_port no longer supported on XS1.
  * CHANGE: Removed the start_read_request() and start_write_request() functions
    from the i2c_slave_callback_if.
  * CHANGE: Removed the start_master_read() and start_master_write() functions
    from the i2c_slave_callback_if.
  * RESOLVED: Fixed timing of i2c master (both single port and multi-port).
  * RESOLVED: Fixed bug with the master not coping with clock stretching on
    start bits.

4.0.2
-----

  * RESOLVED: Make use of Wavedrom in documentation generation offline (fixes
    automated build due to a known Wavevedrom issue where it would generate zero
    size PNG)

4.0.1
-----

  * RESOLVED: Suppressed warning "argument 1 of 'i2c_master_async_aux' slices
    interface preventing analysis of its parallel usage".

4.0.0
-----

  * CHANGE: Register read/write functions are now all MSB first
  * RESOLVED: i2c slave working properly (versions pre 4.0.0 not suitable for
    i2c slave)
  * RESOLVED: Fixed byte ordering of write_reg16_addr8()
  * RESOLVED: Fixed master transmitting on multi-bit port

3.1.6
-----

  * CHANGE: Change title to remove special characters

3.1.5
-----

  * CHANGE: Update app notes

3.1.4
-----

  * CHANGE: Remove invalid app notes

3.1.3
-----

  * CHANGE: Update to source code license and copyright

3.1.2
-----

  * RESOLVED: Fix incorrect reading of r/w bit in slave component

3.1.1
-----

  * CHANGE: Minor user guide updates

3.1.0
-----

  * ADDED: Add support for reading on i2c_master_single-port for xCORE-200
    series.
  * CHANGE: Document reg_read functions more clearly with respect to stop bit
    behavior.

3.0.0
-----

  * CHANGE: Consolidated version, major rework from previous I2C components.

  * Changes to dependencies:

    - lib_logging: Added dependency 2.0.0

    - lib_xassert: Added dependency 2.0.0

