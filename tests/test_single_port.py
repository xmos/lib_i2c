# Copyright (c) 2014-2020, XMOS Ltd, All rights reserved
import xmostest
from i2c_master_checker import I2CMasterChecker
import os

def do_sp_test(stop, speed):
    resources = xmostest.request_resource("xsim")

    binary = 'i2c_sp_test/bin/%(speed)d_%(stop)s_xs2/i2c_sp_test_%(speed)d_%(stop)s_xs2.xe' % {
      'speed' : speed,
      'stop'  : stop
    }

    checker = I2CMasterChecker("tile[0]:XS1_PORT_8A.1",
                               "tile[0]:XS1_PORT_8A.3",
                               tx_data=[0x99, 0x3a, 0xff, 0xaa, 0xbb],
                               expected_speed=speed,
                               # Test some sequences ending with ACK, some with NACK
                               ack_sequence=[True, True, False,
                                             True, True, True, False])

    tester = xmostest.ComparisonTester(open('single_port_test_%s.expect' % stop),
                                     'lib_i2c', 'i2c_master_sim_tests',
                                     'single_port_test',
                                     {'speed':speed, 'stop':stop},
                                     regexp=True)

    if speed == 10:
        tester.set_min_testlevel('nightly')

    sim_args = ['--weak-external-drive']

    xmostest.run_on_simulator(resources['xsim'], binary,
                              simthreads = [checker],
                              simargs=sim_args,
                              suppress_multidrive_messages = True,
                              tester = tester)


def runtest():
    for stop in ['stop', 'no_stop']:
      for speed in [400, 100, 10]:
          do_sp_test(stop, speed)
