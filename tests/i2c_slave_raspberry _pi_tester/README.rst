I2C slave tester using Raspberry Pi 2
=====================================

Summary
-------

This python code is used to test the i2c_slave.xc code where XMOS device
acts as slave and Raspberry Pi acts as master. To use this code, make the
necessary hardware connections and run the i2c_slave code on XMOS device.
Make sure the APP is taking care of the read/write transactions(Exsisting
APP will not work with this script. Check APP for more details) else 
i2c_slave_demo.xc APP present in i2c_slave_raspberry_pi_testser folder can
be used for testing. Copy and paste i2c_slave_raspberry_pi_testser folder 
on Raspberry Pi. Open a terminal and navigate to i2c_slave_raspberry_pi_testser
folder and modify the slave address in i2c_tester.py accordingly and run this 
script on Raspberry Pi's terminal using,

python i2c_tester.py

Features
........

Tests,

  *write_byte
  *read_byte
  *write_word
  *read_word
