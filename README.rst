
:orphan:

####################
lib_i2c: I²C Library
####################

:vendor: XMOS
:version: 6.4.1
:scope: General use
:description: I²C controller and target library
:category: General purpose
:keywords: IO
:devices: xcore.ai, xcore-200

*******
Summary
*******

I²C (Inter-Integrated Circuit) is a multi-master, multi-slave, synchronous, serial communication
protocol used for communication between integrated circuits on the same board. Developed by Philips,
it requires only two lines: the `SDA` (Serial Data Line) for data transfer and `SCL` (Serial Clock
Line) for clock signals. I²C is popular in applications for connecting low-speed peripherals like
sensors, EEPROMs, and ADCs. It supports various data rates, typically up to 3.4 Mbps in Fast Mode
Plus and Ultra-Fast Mode, and allows multiple devices to share the same bus.

``lib_i2c`` contains a software defined, industry-standard, I²C library that allows control of an
I²C bus via `xcore` ports.

``lib_i2c`` provides both controller ("master") and target ("slave") functionality.

The I²C master component can be used by multiple tasks within the `XCORE` device (each addressing
the same or different target devices).

The library can also be used to implement multiple I²C physical interfaces on a single `XCORE`
device simultaneously.

********
Features
********

* I²C controller (master) and I²C target (slave) modes
* Supports speed up to 400 Kb/s (I²C Fast-mode)
* Clock stretching support (optional)
* Synchronous and asynchronous APIs

************
Known issues
************

* The library has functions that wait on SCL high, through either an event or a polling loop.
  If these are called on a system where the pull up isn't present then the application can hang forever.

****************
Development repo
****************

* `lib_i2c <https://www.github.com/xmos/lib_i2c>`_ (https://www.github.com/xmos/lib_i2c)

**************
Required tools
**************

* XMOS XTC Tools: 15.3.1

*********************************
Required libraries (dependencies)
*********************************

* `lib_xassert <https://www.github.com/xmos/lib_xassert>`_ (https://www.github.com/xmos/xassert)

*************************
Related application notes
*************************

The following application notes use this library:

* AN00156: How to use the I²C master library
* AN00157: How to use the I²C slave library
* AN00181: xcore-200 explorer accelerometer demo

*******
Support
*******

This package is supported by XMOS Ltd. Issues can be raised against the software at
`www.xmos.com/support <https://www.xmos.com/support>`_ or using GitHub `issues <https://github.com/xmos/lib_xassert/issues>`_.

