.. |I2C| replace:: I\ :sup:`2`\ C

I2C Library
===========

Summary
-------

A software defined, industry-standard, |I2C| library
that allows you to control an |I2C| bus via xCORE ports.
|I2C| is a two-wire hardware serial
interface, first developed by Philips. The components in the libary
are controlled via C using the XMOS multicore extensions (xC) and
can either act as |I2C| master or slave.

The libary is compatible with multiple slave devices existing on the same
bus. The |I2C| master component can be used by multiple tasks within
the xCORE device (each addressing the same or different slave devices).

Features
........

 * |I2C| master and |I2C| slave modes.
 * Supports speed up to 400 Kb/s.
 * Clock stretching suppoirt.
 * Synchronous and asynchronous APIs for efficient usage of processing cores.


Typical Resource Usage
......................

.. resusage::

  * - configuration: Master
    - globals: port p_scl = XS1_PORT_1A; port p_sda = XS1_PORT_1B;
    - locals: interface i2c_master_if i[1];
    - fn: i2c_master(i, 1, p_scl, p_sda, 100);
    - pins: 2
    - ports: 2 (1-bit)
  * - configuration: Master (single port)
    - globals: port p = XS1_PORT_4A;
    - locals: interface i2c_master_if i[1];
    - fn: i2c_master_single_port(i, 1, p, 100, 0, 0, 0);
    - pins: 2
    - ports: 1 (multi-bit)
    - target: XCORE-200-EXPLORER
  * - configuration: Master (asynchronous)
    - globals: port p_scl = XS1_PORT_1A; port p_sda = XS1_PORT_1B;
    - locals: interface i2c_master_async_if i[1];
    - fn: i2c_master_async(i, 1, p_scl, p_sda, 100, 20);
    - pins: 2
    - ports: 2 (1-bit)
  * - configuration: Master (asynchronous, combinable)
    - globals: port p_scl = XS1_PORT_1A; port p_sda = XS1_PORT_1B;
    - locals: interface i2c_master_async_if i[1];
    - fn: i2c_master_async_comb(i, 1, p_scl, p_sda, 100, 20);
    - pins: 2
    - ports: 2 (1-bit)
  * - configuration: Slave
    - globals: port p_scl = XS1_PORT_1A; port p_sda = XS1_PORT_1B;
    - locals: interface i2c_slave_callback_if i;
    - fn: i2c_slave(i, p_scl, p_sda, 0);
    - pins: 2
    - ports: 2 (1-bit)

Software version and dependencies
.................................

.. libdeps::

Related application notes
.........................

The following application notes use this library:

  * AN00181 - xCORE-200 explorer accelerometer demo
