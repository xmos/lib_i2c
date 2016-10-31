#!/usr/bin/env python
import xmostest
from i2c_master_checker import I2CMasterChecker
import os


def runtest():
    resources = xmostest.request_resource("xsim")

    binary = 'i2c_master_reg_test/bin/i2c_master_reg_test.xe'

    checker = I2CMasterChecker("tile[0]:XS1_PORT_1A",
                               "tile[0]:XS1_PORT_1B",
                               tx_data = [0x99, 0x3A, 0xff, 0x05, 0x11, 0x22],
                               expected_speed = 400,
                               ack_sequence = [False, # NACK header
                                               True, True, False, # NACK before data
                                               True, False, # NACK before data
                                               True, False, # NACK before data
                                               False, # NACK address
                                               True, False, # NACK before data
                                               True, False, # NACK before data
                                               True, True, False # NACK before data
                                              ])

    tester = xmostest.ComparisonTester(open('reg_ops_nack.expect'),
                                     'lib_i2c', 'i2c_master_sim_tests',
                                     'reg_ops_nack_test',
                                     regexp=True)

    xmostest.run_on_simulator(resources['xsim'], binary,
                              simthreads = [checker],
                              simargs=['--weak-external-drive'],
                              suppress_multidrive_messages = True,
                              tester = tester)

