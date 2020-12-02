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

The library can also be used to implement multiple |I2C| physical interfaces
on a single xCORE device simultaneously.

Features
........

 * |I2C| master and |I2C| slave modes.
 * Supports speed up to 400 Kb/s (|I2C| Fast-mode).
 * Clock stretching support.
 * Synchronous and asynchronous APIs for efficient usage of processing cores.


Software version and dependencies
.................................

For a list of direct dependencies, look for DEPENDENT_MODULES in lib_xxx/module_build_info.

Related application notes
.........................

The following application notes use this library:

  * AN00156: How to use the I2C master library
  * AN00157: How to use the I2C slave library
  * AN00181: xCORE-200 explorer accelerometer demo
