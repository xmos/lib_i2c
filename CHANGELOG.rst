lib_i2c change log
==================

6.4.1
-----

  * FIXED: Build warnings (when built with -Wextra)

  * Changes to dependencies:

    - lib_xassert: 4.3.1 -> 4.3.2

6.4.0
-----

  * FIXED: In case of clock stretching, ensure that the required delay happens
    between the slave releasing SCL and the master driving data on SDA.
  * FIXED: Drive data on SDA in open drain mode.

6.3.1
-----

  * CHANGED: Documentation updated

  * Changes to dependencies:

    - lib_xassert: 4.3.0 -> 4.3.1

6.3.0
-----

  * REMOVED: Support for XS1 - Please design with xcore.ai for new projects
  * CHANGED: Build examples and tests using XCommon CMake instead of XCommon

  * Changes to dependencies:

    - lib_xassert: 4.2.0 -> 4.3.0

6.2.0
-----

  * ADDED: Support for XCommon CMake build system
  * REMOVED: Unused dependency lib_logging

  * Changes to dependencies:

    - lib_logging: Removed dependency

    - lib_xassert: 2.0.0 -> 4.2.0

6.1.1
-----

  * RESOLVED: Fixed timing for repeated START condition

6.1.0
-----

  * CHANGED: Use XMOS Public Licence Version 1
  * CHANGED: Rearrange documentation files

6.0.1
-----

  * CHANGED: Pin Python package versions
  * REMOVED: not necessary cpanfile

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

