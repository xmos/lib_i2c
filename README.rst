I2C Library
===========

.. |i2c| replace:: I |-| :sup:`2` |-| C

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
 * Supports speed up to 400 Kb/s.
 * Clock stretching suppoirt.
 * Multi-master/arbitration support.
 * Synchronous and asynchronous APIs for efficient usage of processing cores.

