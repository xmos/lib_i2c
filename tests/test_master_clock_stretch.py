#!/usr/bin/env python
import xmostest
from i2c_master_checker import I2CMasterChecker
import os


def runtest():
    resources = xmostest.request_resource("xsim")

    xmostest.build('i2c_master_test')

    binary = 'i2c_master_test/bin/rx_tx_400/i2c_master_test_rx_tx_400.xe'

    checker = I2CMasterChecker("tile[0]:XS1_PORT_1A",
                               "tile[0]:XS1_PORT_1B",
                               tx_data = [0x99, 0x3A, 0xff],
                               expected_speed = 175,
                               clock_stretch = 5000)

    tester = xmostest.pass_if_matches(open('master_test.expect'),
                                     'lib_i2c', 'i2c_master_sim_tests',
                                      'clock_stretch',
                                     regexp=True)

    xmostest.run_on_simulator(resources['xsim'], binary,
                              simthreads = [checker],
                              simargs=['--weak-external-drive'],
                              suppress_multidrive_messages = True,
                              tester = tester)

