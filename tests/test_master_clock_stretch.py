#!/usr/bin/env python
# Copyright (c) 2014-2018, XMOS Ltd, All rights reserved
import xmostest
from i2c_master_checker import I2CMasterChecker
import os


def do_test(arch, stop):
    resources = xmostest.request_resource("xsim")

    speed = 400
    binary = 'i2c_master_test/bin/rx_tx_%(speed)s_%(stop)s_%(arch)s/i2c_master_test_rx_tx_%(speed)s_%(stop)s_%(arch)s.xe' % {
      'speed' : speed,
      'stop' : stop,
      'arch' : arch,
    }

    checker = I2CMasterChecker("tile[0]:XS1_PORT_1A",
                               "tile[0]:XS1_PORT_1B",
                               tx_data = [0x99, 0x3A, 0xff],
                               expected_speed=170,
                               clock_stretch=5000,
                               ack_sequence=[True, True, False,
                                             True,
                                             True,
                                             True, True, True, False,
                                             True, False])

    tester = xmostest.ComparisonTester(open('master_test_%s.expect' % stop),
                                     'lib_i2c', 'i2c_master_sim_tests',
                                      'clock_stretch',
                                      {'speed' : speed, 'stop' : stop, 'arch' : arch},
                                     regexp=True)

    # vcd_args = '-o test.vcd'
    # vcd_args += ( ' -tile tile[0] -ports -ports-detailed -instructions'
    #   ' -functions -cycles -clock-blocks -pads' )

    # sim_args = ['--weak-external-drive', '--trace-to', 'sim.log']
    # sim_args += [ '--vcd-tracing', vcd_args ]
 
    sim_args = ['--weak-external-drive']

    xmostest.run_on_simulator(resources['xsim'], binary,
                              simthreads = [checker],
                              simargs=sim_args,
                              suppress_multidrive_messages = True,
                              tester = tester)

def runtest():
  for arch in ['xs1', 'xs2']:
    for stop in ['stop', 'no_stop']:
      do_test(arch, stop)
