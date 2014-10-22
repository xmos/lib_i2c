#!/usr/bin/env python
import xmostest
from i2c_master_checker import I2CMasterChecker
import os

def runtest():
    print os.getcwd()
    resources = xmostest.request_resource("xsim")

    xmostest.build('i2c_master_test')

    binary = 'i2c_master_test/bin/rx_tx/i2c_master_test_rx_tx.xe'

    checker = I2CMasterChecker("tile[0]:XS1_PORT_1A",
                               "tile[0]:XS1_PORT_1B",
                               tx_data = [0x99, 0x3A, 0xff],
                               expected_speed = 400)

    tester = xmostest.pass_if_matches(open('master_test.expect'),
                                     'lib_i2c', 'i2c_master_sim_tests',
                                     'basic_test', {'speed':400},
                                     regexp=True)

    xmostest.run_on_simulator(resources['xsim'], binary,
                              simthreads = [checker],
                              simargs=['--weak-external-drive'],
                              suppress_multidrive_messages = True,
                              tester = tester)
