#!/usr/bin/env python
import xmostest
from i2c_master_checker import I2CMasterChecker
import os


def runtest():
    resources = xmostest.request_resource("xsim")

    xmostest.build('i2c_test_repeated_start')

    binary = 'i2c_test_repeated_start/bin/i2c_test_repeated_start.xe'

    checker = I2CMasterChecker("tile[0]:XS1_PORT_1A",
                               "tile[0]:XS1_PORT_1B",
                               expected_speed = 400)

    tester = xmostest.ComparisonTester(open('repeated_start.expect'),
                                     'lib_i2c', 'i2c_master_sim_tests',
                                      'repeated_start',
                                     regexp=True)

    xmostest.run_on_simulator(resources['xsim'], binary,
                              simthreads = [checker],
                              simargs=['--weak-external-drive'],
                              suppress_multidrive_messages = True,
                              tester = tester)

