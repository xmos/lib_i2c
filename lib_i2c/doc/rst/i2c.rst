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

Usage
-----

I2C master synchronous operation
................................

There are two types of interface for |i2c| master components:
synchronous and asynchronous.

The synchronous API provides blocking operation. Whenever a client makes a
read or write call the operation will complete before the client can
move on - this will occupy the core that the client code is running on
until the end of the operation. This method is easy to use, has low
resource use and is very suitable for applications such as setup and
configuration of attached peripherals.

|i2c| master components are instantiated as parallel tasks that run in a
``par`` statement. The application can connect via an interface
connection using the ``i2c_master_if`` interface type:

.. figure:: images/i2c_master_task_diag.*

   I2C master task diagram

For example, the following code instantiates an |i2c| master component
and connect to it::

  port p_scl = XS1_PORT_4C;
  port p_sda = XS1_PORT_1G;
   
  int main(void) {
    i2c_master_if i2c[1];
    par {
      i2c_master(i2c, 1, p_scl, p_sda, 100);
      my_application(i2c[0]);
    }
    return 0;
  }

Note that the connection is an array of interfaces, so several tasks
can connect to the same component instance.

The application can use the client end of the interface connection to
perform |i2c| bus operations e.g.::

  void my_application(client i2c_master_if i2c) {
    uint8_t data[2];
    i2c.rx(0x90, data, 2, 1);
    printf("Read data %d, %d from the bus.\n", data[0], data[1]);
  }

Here the operations such as ``i2c.rx`` will
block until the operation is completed on the bus.
More information on interfaces and tasks can be be found in
the :ref:`XMOS Programming Guide<programming_guide>`. By default the
|i2c| synchronous master mode component does not use any logical cores of its
own. It is a *distributed* task which means it will perform its
function on the logical core of the application task connected to
it (provided the application task is on the same tile).

I2C master asynchronous operation
.................................

The synchronous API will block your application until the bus
operation is complete. In cases where the application cannot afford to
wait for this long the asynchronous API can be used.

The asynchronous API offloads operations to another task. Calls are
provide to initiate reads and writes and notifications are provided
when the operation completes. This API requires more management in the
application but can provide much more efficient operation.
It is particularly suitable for applications where the |i2c| bus is
being used for continuous data transfer.

Setting up an asynchronous |i2c| master component is done in the same
manner as the synchronous component::

  port p_scl = XS1_PORT_4C;
  port p_sda = XS1_PORT_1G;
   
  int main(void) {
    i2c_master_async_if i2c[1];
    par {
      i2c_master_async(i2c, 1, p_scl, p_sda, 100);
      my_application(i2c[0]);
    }
    return 0;
  }

The application can then use the asynchronous API to offload bus
operations to the component. For example, the following code
repeatedly calculates 100 bytes to send over the bus::

  void my_application(client i2c_master_async_if i2c, uin8_t device_addr) {
    uint8_t buffer[100];

    // create and send initial data
    fill_buffer_with_data(buffer);
    i2c.tx(device_addr, buffer, 100, 1);
    while (1) {
      select {
        case i2c.operation_completed():
          i2c_res_t result;
          unsigned num_bytes_sent;
          result = get_tx_result(num_bytes_sent);
          if (result != I2C_SUCCEEDED)
             handle_bus_error();

          // Offload the next 100 bytes data to be sent
          i2c.tx(device_addr, buffer, 100, 1);

          // Calculate the next set of data to go
          fill_buffer_with_data(buffer);
          break;
      }
    }
  }

Here the calculation of ``fill_buffer_with_data`` will overlap with
the sending of data by the other task.

Repeated start bits
...................

The library supports repeated start bits. The ``rx`` and ``tx``
functions allow the application to specify whether to send a stop bit
at the end of the transaction. If this is set to ``0`` then no stop
bit is sent and the next transaction will begin with a repeated start
bit e.g.::

   // Do a tx operation with no stop bit
   i2c.tx(device_addr, data, 2, num_bytes_sent, 0);

   // This operation will begin with a repeated start bit.
   i2c.rx(device_addr, data, 1, 1);


Note that if no stop bit is sent then no other task using the
component can use send or receive data. They will block until a stop
bit is sent.

I2C slave library usage
.......................

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


Master API
----------

All |i2c| master functions can be accessed via the ``i2c.h`` header::

  #include <i2c.h>

You will also have to add ``lib_i2c`` to the
``USED_MODULES`` field of your application Makefile.

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

.. doxygenenum:: i2c_res_t

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

|newpage|

Creating an I2C slave instance
..............................

.. doxygenfunction:: i2c_slave

|newpage|

I2C slave interface
...................

.. doxygeninterface:: i2c_slave_callback_if

|newpage|

|appendix|

Known Issues
------------

There are no known issues with this library.

.. include:: ../../../CHANGELOG.rst
