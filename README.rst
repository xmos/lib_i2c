I2C Libary
----------

A software defined, industry-standard, I2C library
that allows you to control an I2C bus via xCORE ports.
I2C is a two-wire hardware serial
interface, first developed by Philips. The components in the libary
are controlled via C using the XMOS multicore extensions (xC) and
can either act as I2C master or slave.

The libary is compatible with multiple slave devices existing on the same
bus. The I2C master component can be used by multiple tasks within
the xCORE device (each addressing the same or different slave devices).

Features
........

 * I2C master and I2C slave modes.
 * Supports speed of 100 Kb/s or 400 Kb/s.
 * Multiple master/clock stretching support.

Components
...........

 * I2C master
 * I2C master using a single multi-bit xCORE port (reading not supported)
 * I2C slave

