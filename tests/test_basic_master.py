#!/usr/bin/env python
import xmostest
from i2c_master_checker import I2CMasterChecker
import os


def do_master_test(speed, stop):
    resources = xmostest.request_resource("xsim")

    binary = 'i2c_master_test/bin/rx_tx_%(speed)d_%(stop)s/i2c_master_test_rx_tx_%(speed)d_%(stop)s.xe' % {
      'speed' : speed,
      'stop'  : stop
    }

    checker = I2CMasterChecker("tile[0]:XS1_PORT_1A",
                               "tile[0]:XS1_PORT_1B",
                               tx_data = [0x99, 0x3A, 0xff],
                               expected_speed = speed,
                               ack_sequence=[True, True, False,
                                             True,
                                             True,
                                             True, True, True, False,
                                             True, False])

    tester = xmostest.ComparisonTester(open('master_test_%s.expect' % stop),
                                     'lib_i2c', 'i2c_master_sim_tests',
                                     'basic_test', {'speed':speed, 'stop':stop},
                                     regexp=True)

    if speed == 10:
        tester.set_min_testlevel('nightly')

    xmostest.run_on_simulator(resources['xsim'], binary,
                              simthreads = [checker],
                              simargs=sim_args,
                              suppress_multidrive_messages = True,
                              tester = tester)

def runtest():
    for stop in ['stop', 'no_stop']:
      for speed in [400, 100, 10]:
        do_master_test(speed, stop)
