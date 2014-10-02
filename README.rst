I2C Library
===========

.. |i2c| replace:: I |-| :sup:`2` |-| C

.. rheader::

   I2C |version|

I2C Libary
----------

A software defined, industry-standard, |i2c| library
that allows you to control an |i2c| bus via xCORE ports.
|i2c| is a two-wire hardware serial
interface, first developed by Philips. The components in the libary
are controlled via C using the XMOS multicore extensions (xC) and
can either act as |i2c| master or slave.

The libary is compatible with multiple slave devices existing on the same
bus. The |i2c| master component can be used by multiple tasks within
the xCORE device (each addressing the same or different slave devices).

Features
........

 * |i2c| master and |i2c| slave modes.
 * Supports speed of 100 Kb/s or 400 Kb/s.
 * Multiple master/clock stretching support.

Components
...........

 * |i2c| master
 * |i2c| master using a single multi-bit xCORE port (reading not supported)
 * |i2c| slave

Resource Usage
..............

.. list-table::
   :header-rows: 1
   :class: wide vertical-borders horizontal-borders

   * - Component
     - Pins
     - Ports
     - Clock Blocks
     - Ram
     - Logical cores
   * - Master
     - 2
     - 2 (1-bit)
     - 0
     - ~1.2K
     - 0
   * - Master (multi-master disabled)
     - 2
     - 2 (1-bit)
     - 0
     - ~0.9K
     - 0
   * - Master (single port)
     - 2
     - 1 (multi-bit)
     - 0
     - ~1.1K
     - 0
   * - Slave
     - 2
     - 2 (1-bit)
     - 0
     - ~1.5K
     - 1

Software version and dependencies
.................................

This document pertains to version |version| of the I2C library. It is
intended to be used with version 13.x of the xTIMEcomposer studio tools.

The library does not have any dependencies (i.e. it does not rely on any
other libraries).

Related application notes
.........................

The following application notes use this library:

  * AN00052 - How to use the I2C master component
  * AN00057 - How to use the I2C slave component
