#!/usr/bin/env python
import xmostest
from i2c_master_checker import I2CMasterChecker

def runtest():
    resources = xmostest.request_resource("xsim")

    binary = 'i2c_master_test/bin/tx_only/i2c_master_test_tx_only.xe'

    checker = I2CMasterChecker("tile[0]:XS1_PORT_1A",
                               "tile[0]:XS1_PORT_1B",
                               tx_data = [0x99, 0x3A, 0xff],
                               expected_speed = 400,
                               ack_sequence=[True, True, True,
                                             True, True, False,
                                             False, True])

    tester = xmostest.ComparisonTester(open('ack_test.expect'),
                                      'lib_i2c', 'i2c_master_sim_tests',
                                      'ack_test', {'speed':400},
                                      regexp=True)

    xmostest.run_on_simulator(resources['xsim'], binary,
                              simthreads = [checker],
                              simargs=['--weak-external-drive'],
                              suppress_multidrive_messages = True,
                              tester = tester)
