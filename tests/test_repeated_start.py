# Copyright (c) 2014-2018, XMOS Ltd, All rights reserved
import xmostest
from i2c_master_checker import I2CMasterChecker
import os

def do_test(arch):
    resources = xmostest.request_resource("xsim")

    binary = 'i2c_test_repeated_start/bin/%(arch)s/i2c_test_repeated_start_%(arch)s.xe' % {
      'arch'  : arch
    }

    checker = I2CMasterChecker("tile[0]:XS1_PORT_1A",
                               "tile[0]:XS1_PORT_1B",
                               expected_speed=400)

    sim_args = ['--weak-external-drive']

    tester = xmostest.ComparisonTester(open('repeated_start.expect'),
                                     'lib_i2c', 'i2c_master_sim_tests',
                                      'repeated_start', {'arch' : arch},
                                     regexp=True)

    xmostest.run_on_simulator(resources['xsim'], binary,
                              simthreads = [checker],
                              simargs=sim_args,
                              suppress_multidrive_messages = True,
                              tester = tester)

def runtest():
  for arch in ['xs1', 'xs2', 'xcoreai']:
    do_test(arch)
