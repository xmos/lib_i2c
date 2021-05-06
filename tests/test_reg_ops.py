# Copyright 2014-2021 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
import xmostest
from i2c_master_checker import I2CMasterChecker
import os


def do_test(arch):
    resources = xmostest.request_resource("xsim")

    binary = 'i2c_master_reg_test/bin/%(arch)s/i2c_master_reg_test_%(arch)s.xe' % {
      'arch' : arch
    }

    checker = I2CMasterChecker("tile[0]:XS1_PORT_1A",
                               "tile[0]:XS1_PORT_1B",
                               tx_data = [0x99, 0x3A, 0xff, 0x05, 0xee, 0x06],
                               expected_speed = 400,
                               ack_sequence=[True, True, False,
                                             True, True, True, False,
                                             True, True, True, True, False,
                                             True, True, True, False,
                                             True, True,
                                             True, True, True,
                                             True, True, True, True,
                                             True, True, True])

    tester = xmostest.ComparisonTester(open('reg_test.expect'),
                                     'lib_i2c', 'i2c_master_sim_tests',
                                     'reg_ops_test',
                                     {'arch' : arch},
                                     regexp=True)

    xmostest.run_on_simulator(resources['xsim'], binary,
                              simthreads=[checker],
                              simargs=['--weak-external-drive'],
                              suppress_multidrive_messages=True,
                              tester=tester)

def runtest():
  for arch in ['xs1', 'xs2', 'xcoreai']:
    do_test(arch)
