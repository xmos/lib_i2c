# Copyright (c) 2014-2021, XMOS Ltd, All rights reserved
import xmostest
from i2c_slave_checker import I2CSlaveChecker
import os


def do_slave_test(arch, speed, level):
    resources = xmostest.request_resource("xsim")

    binary = 'i2c_slave_test/bin/%(arch)s/i2c_slave_test_%(arch)s.xe' % {
      'arch' : arch
    }
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

    tester.set_min_testlevel(level)

    xmostest.run_on_simulator(resources['xsim'], binary,
                              simthreads = [checker],
                              simargs=['--weak-external-drive'],
                              suppress_multidrive_messages = True,
                              tester = tester)

def runtest():
    for arch in ['xs1', 'xcoreai']:
        do_slave_test(arch, 400, 'smoke')
        do_slave_test(arch, 100, 'nightly')
        do_slave_test(arch, 10, 'nightly')
