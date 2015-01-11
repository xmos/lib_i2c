#!/usr/bin/env python
import xmostest
from i2c_master_checker import I2CMasterChecker
import os

def runtest():
    resources = xmostest.request_resource("xsim")


    speed = 100
    build_config = "interfere"

    xmostest.build('i2c_master_async_test', build_config = build_config)

    binary = 'i2c_master_async_test/bin/%(config)s/i2c_master_async_test_%(config)s.xe' % {'config':build_config}

    checker = I2CMasterChecker("tile[0]:XS1_PORT_1A",
                               "tile[0]:XS1_PORT_1B",
                               tx_data = [0x99, 0x3A, 0xff],
                               expected_speed = None,
                               ack_sequence=[True, True, False,
                                             True,
                                             True,
                                             True, True, True, False,
                                             True, False])

    tester = xmostest.ComparisonTester(open('master_test.expect'),
                                     'lib_i2c', 'i2c_master_sim_tests',
                                     'async_interference_test', {'speed':str(speed)},
                                     regexp=True)

    xmostest.run_on_simulator(resources['xsim'], binary,
                              simthreads = [checker],
                              simargs=['--weak-external-drive'],
                              suppress_multidrive_messages = True,
                              tester = tester)
