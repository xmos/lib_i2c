.. include:: ../../../README.rst

Hardware characteristics
------------------------

The |i2c| protocol requires two wires to be connected to the xCORE
device: a clock line and data line as shown in the table below.
Both lines can be bi-directional and of
open-drain design.

.. _i2c_wire_table:

.. list-table:: I2C data wires
     :class: vertical-borders horizontal-borders

     * - *SDA*
       - Data line that transmits data in both directions.
     * - *SCL*
       - Clock line, usually driven by the master unless clock
         stretching is enabled.

Master configuration
....................

When using the component in master mode it can be configured in normal
or single-port mode. In single-port mode reading from the bus is not
supported. In normal mode the xCORE should be connected as shown in
:ref:`i2c_master_mode_hw_diagg`.

.. _i2c_master_mode_hw_diagg:

.. figure:: images/i2c-master-schem-crop.*
   :width: 50%

   External connections in master configuration.

In single-port mode the xCORE should be connected as shown in
:ref:`i2c_master_mode_hw_sp_diagg`.

.. _i2c_master_mode_hw_sp_diagg:

.. figure:: images/i2c-master-schem-sp-crop.*
   :width: 50%

   External connections in master configuration (single port).

In both cases, the SCL and SDA lines require 1K pull-up resistors be
present.


Slave configuration
....................

When using the component in slave mode the xCORE should be connected as shown in
:ref:`i2c_slave_mode_hw_diag`. The SCL and SDA lines require 1K
pull-up resistors be present.

.. _i2c_slave_mode_hw_diag:

.. figure:: images/i2c-master-schem-crop.*
   :width: 50%

   External connections in slave configuration

Master API
----------

All |i2c| master functions can be accessed via the ``i2c.h`` header::

  #include <i2c.h>

You will also have to add ``lib_i2c`` to the
``USED_MODULES`` field of your application Makefile.

|i2c| master components are instantiated as parallel tasks that run in a
``par`` statement. The application can connect via an interface
connection.

.. figure:: images/i2c_master_task_diag.*

   I2C master task diagram

For example, the following code instantiates an |i2c| master component
and connect to it::

  port p_scl = XS1_PORT_4C;
  port p_sda = XS1_PORT_1G;
   
  int main(void) {
    i2c_master_if i2c[1];
    par {
      i2c_master(i2c, 1, p_scl, p_sda, 100, I2C_ENABLE_MULTIMASTER);
      my_application(i2c[0]);
    }
    return 0;
  }

Note that the connection is an array of interfaces, so several tasks
can connect to the same component instance.

The application can use the client end of the interface connection to
perform |i2c| bus operations e.g.::

  void my_application(client i2c_master_if i2c) {
    i2c.write_reg(0x90, 0x07, 0x12);
    i2c.write_reg(0x90, 0x08, 0x78);
    unsigned char data = i2c.read_reg(0x90, 0x07);
    printf("Read data %x from addr 0x90,0x07 (should be 0x12)\n", data);
  }

More information on interfaces and tasks can be be found in
the :ref:`XMOS Programming Guide<programming_guide>`. By default the
|i2c| master mode component does not use any logical cores of its
own. It is a *distributed* task which means it will perform its
function on the logical core of the application task connected to
it. If, however, the tasks are on different hardware
tiles then the |i2c| master component will need a core to run on.

|newpage|

Synchronous vs. Asynchronous operation
......................................

There are two types of interface for |i2c| master components:
synchronous and asynchronous.

The synchronous API provides blocking operation. Whenever a client makes a
read or write call the operation will complete before the client can
move on - this will occupy the core that the client code is running on
until the end of the operation. This method is easy to use, has low
resource use and is very suitable for applications such as setup and
configuration of attached peripherals.

The asynchronous API offloads operations to another task. Calls are
provide to initiate reads and writes and notifications are provided
when the operation completes. This API is trickier to use but can
provide more efficient operation. It is suitable for applications
where the |i2c| bus is being used for continuous data transfer.

|newpage|

Creating an I2C master instance
...............................

.. doxygenfunction:: i2c_master

|newpage|

.. doxygenfunction:: i2c_master_single_port

|newpage|

.. doxygenfunction:: i2c_master_async

|newpage|

I2C master supporting typedefs
..............................

.. doxygenenum:: i2c_write_res_t

|newpage|

I2C master interface
....................

.. doxygeninterface:: i2c_master_if

|newpage|

I2C master asynchronous interface
.................................

.. doxygeninterface:: i2c_master_async_if


Slave API
---------

All |i2c| slave functions can be accessed via the ``i2c.h`` header::

  #include <i2c.h>

You will also have to add ``lib_i2c`` to the
``USED_MODULES`` field of your application Makefile.


|i2c| slave components are instantiated as parallel tasks that run in a
``par`` statement. The application can connect via an interface
connection.

.. figure:: images/i2c_slave_task_diag.pdf

   I2C slave task diagram

For example, the following code instantiates an |i2c| slave component
and connect to it::

  port p_scl = XS1_PORT_4C;
  port p_sda = XS1_PORT_1G;
   
  int main(void) {
    i2c_slave_if i2c;
    par {
      i2c_slave(i2c, p_scl, p_sda, 0x3b, 2);
      my_application(i2c);
    }
    return 0;
  }

The slave component acts as the client of the interface
connection. This means it can "callback" to the application to respond
to requests from the bus master. For example, the ``my_application``
function above needs to respond to the calls e.g.::

  void my_application(server i2c_slave_if i2c)
  {
    while (1) {
      select {
      case i2c.master_performed_write(uint8_t data[n], size_t n):
         // handle write to device here
         ...
         break;
      case i2c.master_requests_read(uint8_t data[n], size_t n):
         // handle read from device here
         ...
         break;
      }
    }
  }


More information on interfaces and tasks can be be found in the :ref:`XMOS Programming Guide<programming_guide>`.

|newpage|

Creating an I2C slave instance
..............................

.. doxygenfunction:: i2c_slave

|newpage|

I2C slave interface
...................

.. doxygeninterface:: i2c_slave_callback_if

|newpage|
