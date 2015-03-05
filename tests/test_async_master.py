#!/usr/bin/env python
import xmostest
from i2c_master_checker import I2CMasterChecker
import os


def do_master_test(speed, comb):
    resources = xmostest.request_resource("xsim")

    if comb:
        build_config = "comb_%d" % speed
        config = {'speed':speed,'impl':'comb'}
    else:
        build_config = str(speed)
        config = {'speed':speed,'impl':'noncomb'}

    binary = 'i2c_master_async_test/bin/%(config)s/i2c_master_async_test_%(config)s.xe' % {'config':build_config}

    checker = I2CMasterChecker("tile[0]:XS1_PORT_1A",
                               "tile[0]:XS1_PORT_1B",
                               tx_data = [0x99, 0x3A, 0xff],
                               expected_speed = speed,
                               ack_sequence=[True, True, False,
                                             True,
                                             True,
                                             True, True, True, False,
                                             True, False])

    tester = xmostest.ComparisonTester(open('master_test.expect'),
                                     'lib_i2c', 'i2c_master_sim_tests',
                                     'async_basic_test', config,
                                     regexp=True)

    if speed == 10:
        tester.set_min_testlevel('nightly')

    xmostest.run_on_simulator(resources['xsim'], binary,
                              simthreads = [checker],
                              simargs=['--weak-external-drive'],
                              suppress_multidrive_messages = True,
                              tester = tester)

def runtest():
    do_master_test(400, False)
    do_master_test(100, False)
    do_master_test(10, False)
    do_master_test(100, True)
    do_master_test(10, True)
