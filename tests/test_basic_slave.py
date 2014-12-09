import xmostest
from i2c_slave_checker import I2CSlaveChecker
import os


def do_slave_test(speed):
    resources = xmostest.request_resource("xsim")

    xmostest.build('i2c_slave_test')

    binary = 'i2c_slave_test/bin/i2c_slave_test.xe'

    checker = I2CSlaveChecker("tile[0]:XS1_PORT_1A",
                              "tile[0]:XS1_PORT_1B",
                              tsequence =
                              [("w", 0x3c, [0x33, 0x44, 0x3]),
                               ("r", 0x3c, 3),
                               ("w", 0x3c, [0x99]),
                               ("w", 0x44, [0x33]),
                               ("r", 0x3c, 1),
                               ("w", 0x3c, [0x22, 0xff])],
                               speed = speed)

    tester = xmostest.ComparisonTester(open('basic_slave_test.expect'),
                                     'lib_i2c', 'i2c_slave_sim_tests',
                                     'basic_test', {'speed':speed},
                                     regexp=True)

    if speed == 10:
        tester.set_min_testlevel('nightly')

    xmostest.run_on_simulator(resources['xsim'], binary,
                              simthreads = [checker],
                              simargs=['--weak-external-drive'],
                              suppress_multidrive_messages = True,
                              tester = tester)

def runtest():
    do_slave_test(400)
    do_slave_test(100)
    do_slave_test(10)
