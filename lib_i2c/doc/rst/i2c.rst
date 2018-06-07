.. include:: ../../../README.rst

External signal description
---------------------------

All signals are designed to comply with the timings in the |I2C|
specification found here:

http://www.nxp.com/documents/user_manual/UM10204.pdf

Note that the following optional parts of the |I2C| specification are *not*
supported:

  * Multi-master arbitration
  * 10-bit slave addressing
  * General call addressing
  * Software reset
  * START byte
  * Device ID
  * Fast-mode Plus, High-speed mode, Ultra Fast-mode

|I2C| consists of two signals: a clock line (SCL) and a data line
(SDA). Both these signals are *open-drain* and require external
resistors to pull the line up if no device is driving the signal
down. The correct value for the resistors can be found in the |I2C|
specification.

.. figure:: images/i2c_open_drain.pdf
   :width: 60%

   |I2C| open-drain layout

Transactions on the line occur between a *master* and a *slave*. The
master always drives the clock (though the slave can delay the
transaction at any point by holding the clock line down). The master
initiates a transaction with a start bit (consisting of driving the
data line from high to low whilst the clock line is high). It will
then clock out a seven-bit device address followed by a read/write
bit. The master will then drive one more clock pulse during which the
slave can either ACK (drive the line low), accepting the transaction
or NACK (leave the line high). This sequence is shown in :ref:`i2c_transaction_start`.

.. _i2c_transaction_start:

.. figure:: images/transaction_start.png
   :width: 100%

   |I2C| transaction start

If the read/write bit of the transaction start is 1 then the master
will execute a sequence of reads. Each read consists of the master
driving the clock whilst the slave drives the data for 8-bits (most
significant bit first). At the end of each byte, the master drives
another clock pulse and will either drive either an ACK (0) or
NACK (1) signal on the data line. When the master drives a NACK
signal, the sequence of reads is complete. A read byte sequence is
show in :ref:`i2c_read_byte`

.. _i2c_read_byte:

.. figure:: images/read_byte.png
   :width: 100%

   |I2C| read byte

|newpage|

If the read/write bit of the transaction start is 0 then the master
will execute a sequence of writes. Each write consists of the master
driving the clock whilst and also driving the data for 8-bits (most
significant bit first). At the end of each byte, the master drives
another clock pulse and the slave will either drive either an ACK (0)
(signalling that it can accept more data) or a NACK (1) (signalling
that it cannot accept more data) on the data line. After the ACK/NACK
signal, the master can complete the transaction with a stop bit or
repeated start. A write byte sequence is show in :ref:`i2c_write_byte`

.. _i2c_write_byte:

.. figure:: images/write_byte.png
   :width: 100%

   |I2C| write byte

After a transaction is complete, the master may start a new
transaction (a *repeated start*) or will send a
stop bit consisting of releasing the data line so that it floats from low to high whilst
the clock line is high (see :ref:`i2c_stop_bit`).

.. _i2c_stop_bit:

.. figure:: images/stop_bit.png
   :width: 100%

   |I2C| stop bit

|newpage|

Connecting to the xCORE device
..............................

When the xCORE is the |I2C| master, the normal configuration is to
connect the clock and data lines to different 1-bit ports as shown in
:ref:`i2c_master_1_bit`.

.. _i2c_master_1_bit:

.. figure:: images/i2c_master_1_bit.pdf
  :width: 40%

  |I2C| master (1-bit ports)

It is possible to connect both lines to different bits of a multi-bit
port as shown in :ref:`i2c_master_n_bit`. This is useful if other
constraints limit the use of one bit ports. However the following
should be taken into account:

  * On L-series and U-series devices in this configuration,
    the xCORE can only perform write transactions to the |I2C| bus.
  * On L-series and U-series clock stretching
    is not supported in this configuration.
  * The other bits of the multi-bit port cannot be used for any other
    function.

The restrictions on reading and clock stretching do not apply to
xCORE-200 devices.

.. _i2c_master_n_bit:

.. figure:: images/i2c_master_n_bit.pdf
  :width: 40%

  |I2C| master (single n-bit port)

When the xCORE is acting as |I2C| slave the two lines *must* be
connected to two 1-bit ports (as shown in :ref:`i2c_slave_connection`).

.. _i2c_slave_connection:

.. figure:: images/i2c_slave.pdf
  :width: 40%

  |I2C| slave connection

Usage
-----

|I2C| master synchronous operation
..................................

There are two types of interface for |I2C| masters: synchronous and asynchronous.

The synchronous API provides blocking operation. Whenever a client makes a
read or write call the operation will complete before the client can
move on - this will occupy the core that the client code is running on
until the end of the operation. This method is easy to use, has low
resource use and is very suitable for applications such as setup and
configuration of attached peripherals.

|I2C| masters are instantiated as parallel tasks that run in a
``par`` statement. For synchronous operation, the application
can connect via an interface connection using the ``i2c_master_if``
interface type:

.. figure:: images/i2c_master_task_diag.*

   |I2C| master task diagram

For example, the following code instantiates an |I2C| master and connects to it::

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

For the single multi-bit port version of |I2C| the
top level instantiation would look like::

  port p_i2c = XS1_PORT_4C;

  int main(void) {
    i2c_master_if i2c[1];
    par {
      i2c_master_single_port(i2c, 1, p_i2c, 100, 1, 3, 0);
      my_application(i2c[0]);
    }
    return 0;
  }

Note that the connection is an array of interfaces, so several tasks
can connect to the same master.

|newpage|

The application can use the client end of the interface connection to
perform |I2C| bus operations e.g.::

  void my_application(client i2c_master_if i2c) {
    uint8_t data[2];
    i2c.read(0x90, data, 2, 1);
    printf("Read data %d, %d from the bus.\n", data[0], data[1]);
  }

Here the operations such as ``i2c.read`` will
block until the operation is completed on the bus.
More information on interfaces and tasks can be be found in
the :ref:`XMOS Programming Guide<programming_guide>`. By default the
|I2C| synchronous master mode component does not use any logical cores of its
own. It is a *distributed* task which means it will perform its
function on the logical core of the application task connected to
it (provided the application task is on the same tile as the |I2C| ports).

|I2C| master asynchronous operation
...................................

The synchronous API will block your application until the bus
operation is complete. In cases where the application cannot afford to
wait for this long the asynchronous API can be used.

The asynchronous API offloads operations to another task. Calls are
provided to initiate reads and writes. Notifications are provided
when the operation completes. This API requires more management in the
application but can provide much more efficient operation.
It is particularly suitable for applications where the |I2C| bus is
being used for continuous data transfer.

Setting up an asynchronous |I2C| master component is done in the same
manner as the synchronous component::

  port p_scl = XS1_PORT_4C;
  port p_sda = XS1_PORT_1G;

  static const size_t buffer_bytes = 100;
  static const uint8_t target_device = 0x3c;

  int main(void) {
    i2c_master_async_if i2c[1];
    par {
      i2c_master_async(i2c, 1, p_scl, p_sda, 100, buffer_bytes);
      my_application(i2c[0], target_device);
    }
    return 0;
  }

|newpage|

The application can then use the asynchronous API to offload bus
operations to the |I2C| master. For example, the following code
repeatedly calculates *buffer_bytes* bytes to send over the bus::

  void my_application(client i2c_master_async_if i2c, uin8_t device_addr) {
    uint8_t buffer[buffer_bytes];

    // create and send initial data
    my_application_fill_buffer(buffer);
    i2c.write(device_addr, buffer, buffer_bytes, 1);

    // Start calculating the next block of data
    my_application_fill_buffer(buffer);

    while (1) {
      select {
        case i2c.operation_complete():
          size_t num_bytes_sent;
          i2c_res_t result = i2c.get_write_result(num_bytes_sent);
          if (num_bytes_sent != buffer_bytes) {
             my_application_handle_bus_error(result);
          }

          // Offload the next data bytes data to be sent
          i2c.write(device_addr, buffer, buffer_bytes, 1);

          // Calculate the next set of data to go
          my_application_fill_buffer(buffer);

          break;
      }
    }
  }

Here the calculation of ``my_application_fill_buffer`` will overlap with
the sending of data by the other task.

Repeated start bits
...................

The library supports repeated start bits. The ``read`` and ``write``
functions allow the application to specify whether to send a stop bit
at the end of the transaction. If this is set to ``0`` then no stop
bit is sent and the next transaction will begin with a repeated start
bit e.g.::

   // Do a write operation with no stop bit
   i2c.write(device_addr, data, 2, num_bytes_sent, 0);

   // This operation will begin with a repeated start bit
   i2c.read(device_addr, data, 1, 1);

Note that if no stop bit is sent then no other client using the
same |I2C| master can send or receive data. They will block until a stop
bit is sent.

|newpage|

|I2C| slave library usage
.........................

|I2C| slaves are instantiated as parallel tasks that run in a
``par`` statement. The application can connect via an interface
connection.

.. figure:: images/i2c_slave_task_diag.pdf

   |I2C| slave task diagram

For example, the following code instantiates an |I2C| slave
and connects to it::

  port p_scl = XS1_PORT_4C;
  port p_sda = XS1_PORT_1G;

  int main(void) {
    i2c_slave_callback_if i2c;
    par {
      i2c_slave(i2c, p_scl, p_sda, 0x3b);
      my_application(i2c);
    }
    return 0;
  }

|newpage|

The slave acts as the client of the interface
connection. This means it can "callback" to the application to respond
to requests from the bus master. For example, the ``my_application``
function above needs to respond to the calls e.g.::

  void my_application(server i2c_slave_callback_if i2c)
  {
    while (1) {
      select {
      case i2c.master_requests_read() -> i2c_slave_ack_t response:
        response = I2C_SLAVE_ACK;
        break;
      case i2c.master_requests_write() -> i2c_slave_ack_t response:
        response = I2C_SLAVE_ACK;
        break;
      case i2c.master_sent_data(uint8_t data) -> i2c_slave_ack_t response:
         // handle write to device here, set response to NACK for the
         // last byte of data in the transaction.
         ...
         break;
      case i2c.master_requires_data() -> uint8_t data:
         // handle read from device here
         ...
         break;
      case i2c.stop_bit():
         break;
      }
    }
  }


More information on interfaces and tasks can be be found in the :ref:`XMOS Programming Guide<programming_guide>`.


Master API
----------

All |I2C| master functions can be accessed via the ``i2c.h`` header::

  #include <i2c.h>

You will also have to add ``lib_i2c`` to the
``USED_MODULES`` field of your application Makefile.

Creating an |I2C| master instance
.................................

.. doxygenfunction:: i2c_master

|newpage|

.. doxygenfunction:: i2c_master_single_port

|newpage|

.. doxygenfunction:: i2c_master_async

|newpage|

|I2C| master supporting typedefs
................................

.. doxygenenum:: i2c_res_t

.. doxygenenum:: i2c_regop_res_t

|newpage|

|I2C| master synchronous interface
..................................

.. doxygeninterface:: i2c_master_if

|newpage|

|I2C| master asynchronous interface
...................................

.. doxygeninterface:: i2c_master_async_if

Slave API
---------

All |I2C| slave functions can be accessed via the ``i2c.h`` header::

  #include <i2c.h>

You will also have to add ``lib_i2c`` to the
``USED_MODULES`` field of your application Makefile.

|newpage|

Creating an |I2C| slave instance
................................

.. doxygenfunction:: i2c_slave

|newpage|

|I2C| slave interface
.....................

.. doxygeninterface:: i2c_slave_callback_if

|newpage|

|appendix|

Known Issues
------------

There are no known issues with this library.


.. include:: ../../../CHANGELOG.rst
