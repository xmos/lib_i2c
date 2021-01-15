# Copyright (c) 2014-2021, XMOS Ltd, All rights reserved
import xmostest
from i2c_master_checker import I2CMasterChecker
import os

def do_master_test(arch, speed, impl, stop):
    resources = xmostest.request_resource("xsim")

    binary = 'i2c_master_async_test/bin/%(impl)s_%(speed)s_%(arch)s_%(stop)s/i2c_master_async_test_%(impl)s_%(speed)s_%(arch)s_%(stop)s.xe' % {
      'impl' : impl,
      'speed' : speed,
      'arch' : arch,
      'stop' : stop
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
                                     'async_basic_test',
                                     {'speed' : speed, 'impl' : impl, 'arch' : arch, 'stop' : stop},
                                     regexp=True)

    if speed == 10:
        tester.set_min_testlevel('nightly')

    xmostest.run_on_simulator(resources['xsim'], binary,
                              simthreads = [checker],
                              simargs=['--weak-external-drive'],
                              suppress_multidrive_messages = True,
                              tester = tester)

def runtest():
  for arch in ['xs1', 'xs2', 'xcoreai']:
    for stop in ['stop', 'no_stop']:
      do_master_test(arch, 400, 'non_comb', stop)
      for impl in ['comb', 'non_comb']:
        for speed in [100, 10]:
          do_master_test(arch, speed, impl, stop)
