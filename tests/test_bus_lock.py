# Copyright (c) 2014-2020, XMOS Ltd, All rights reserved
import xmostest
from i2c_master_checker import I2CMasterChecker
import os


def do_test():
    resources = xmostest.request_resource("xsim")

    binary = 'i2c_test_locks/bin/i2c_test_locks.xe'

    speed = 400

    checker = I2CMasterChecker("tile[0]:XS1_PORT_1A",
                               "tile[0]:XS1_PORT_1B",
                               expected_speed=speed)

    tester = xmostest.ComparisonTester(open('lock_test.expect'),
                                     'lib_i2c', 'i2c_master_sim_tests',
                                     'bus_locks',
                                     {'speed' : speed},
                                     regexp=True)

    xmostest.run_on_simulator(resources['xsim'], binary,
                              simthreads = [checker],
                              simargs=['--weak-external-drive'],
                              suppress_multidrive_messages=True,
                              tester=tester)

def runtest():
  do_test()
