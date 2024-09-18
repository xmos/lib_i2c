Typical Resource Usage
......................

.. resusage::

  * - configuration: Master
    - globals: port p_scl = XS1_PORT_1A; port p_sda = XS1_PORT_1B;
    - locals: interface i2c_master_if i[1];
    - fn: i2c_master(i, 1, p_scl, p_sda, 100);
    - pins: 2
    - ports: 2 (1-bit)
    - flags:
    - target: XCORE-AI-EXPLORER
  * - configuration: Master (single port)
    - globals: port p = XS1_PORT_4A;
    - locals: interface i2c_master_if i[1];
    - fn: i2c_master_single_port(i, 1, p, 100, 0, 0, 0);
    - pins: 2
    - ports: 1 (multi-bit)
    - target: XCORE-AI-EXPLORER
    - flags:
  * - configuration: Master (asynchronous)
    - globals: port p_scl = XS1_PORT_1A; port p_sda = XS1_PORT_1B;
    - locals: interface i2c_master_async_if i[1];
    - fn: i2c_master_async(i, 1, p_scl, p_sda, 100, 20);
    - pins: 2
    - ports: 2 (1-bit)
    - target: XCORE-AI-EXPLORER
    - flags:
  * - configuration: Master (asynchronous, combinable)
    - globals: port p_scl = XS1_PORT_1A; port p_sda = XS1_PORT_1B;
    - locals: interface i2c_master_async_if i[1];
    - fn: i2c_master_async_comb(i, 1, p_scl, p_sda, 100, 20);
    - pins: 2
    - ports: 2 (1-bit)
    - target: XCORE-AI-EXPLORER
    - flags:
  * - configuration: Slave
    - globals: port p_scl = XS1_PORT_1A; port p_sda = XS1_PORT_1B;
    - locals: interface i2c_slave_callback_if i;
    - fn: i2c_slave(i, p_scl, p_sda, 0);
    - pins: 2
    - ports: 2 (1-bit)
    - target: XCORE-AI-EXPLORER
    - flags:
