#!/usr/bin/env python
import xmostest
from i2c_master_checker import I2CMasterChecker
import os


def runtest():
    resources = xmostest.request_resource("xsim")

    xmostest.build('i2c_master_test')

    binary = 'i2c_master_reg_test/bin/i2c_master_reg_test.xe'

    checker = I2CMasterChecker("tile[0]:XS1_PORT_1A",
                               "tile[0]:XS1_PORT_1B",
                               tx_data = [0x99, 0x3A, 0xff, 0x05],
                               expected_speed = 400,
                               ack_sequence = [False,
                                               True, True, False,
                                               True, False,
                                               False,
                                               True, False,
                                               True, True, False])

    tester = xmostest.pass_if_matches(open('reg_ops_nack.expect'),
                                     'lib_i2c', 'i2c_master_sim_tests',
                                     'reg_ops_nack_test',
                                     regexp=True)

    xmostest.run_on_simulator(resources['xsim'], binary,
                              simthreads = [checker],
                              simargs=['--weak-external-drive'],
                              suppress_multidrive_messages = True,
                              tester = tester)

