# Copyright (c) 2014-2018, XMOS Ltd, All rights reserved
#!/usr/bin/env python
import xmostest
from i2c_master_checker import I2CMasterChecker
import os

def do_master_test(arch, stop):
    resources = xmostest.request_resource("xsim")

    speed = 100

    binary = 'i2c_master_async_test/bin/interfere_%(arch)s_%(stop)s/i2c_master_async_test_interfere_%(arch)s_%(stop)s.xe' % {
     'arch' : arch,
     'stop' : stop,
    }

    checker = I2CMasterChecker("tile[0]:XS1_PORT_1A",
                               "tile[0]:XS1_PORT_1B",
                               tx_data = [0x99, 0x3A, 0xff],
                               expected_speed = None,
                               ack_sequence=[True, True, False,
                                             True,
                                             True,
                                             True, True, True, False,
                                             True, False])

    tester = xmostest.ComparisonTester(open('master_test_%s.expect' % stop),
                                     'lib_i2c', 'i2c_master_sim_tests',
                                     'async_interference_test',
                                     {'speed':speed, 'arch' : arch, 'stop' : stop},
                                     regexp=True)

    sim_args = ['--weak-external-drive']

    xmostest.run_on_simulator(resources['xsim'], binary,
                              simthreads = [checker],
                              simargs=sim_args,
                              suppress_multidrive_messages = True,
                              tester = tester)

def runtest():
  for arch in ['xs1', 'xs2']:
    for stop in ['stop', 'no_stop']:
      do_master_test(arch, stop)
