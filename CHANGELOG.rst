I2C library change log
======================

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

