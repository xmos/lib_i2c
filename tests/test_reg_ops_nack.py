#!/usr/bin/env python
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
                               tx_data=[0x99, 0x3A, 0xff, 0x05, 0x11, 0x22],
                               expected_speed=400,
                               ack_sequence=[False, # NACK header
                                               True, True, False, # NACK before data
                                               True, False, # NACK before data
                                               True, False, # NACK before data
                                               False, # NACK address
                                               True, False, # NACK before data
                                               True, False, # NACK before data
                                               True, True, False # NACK before data
                                            ])

    tester = xmostest.ComparisonTester(open('reg_ops_nack.expect'),
                                     'lib_i2c', 'i2c_master_sim_tests',
                                     'reg_ops_nack_test',
                                     {'arch' : arch},
                                     regexp=True)

    xmostest.run_on_simulator(resources['xsim'], binary,
                              simthreads=[checker],
                              simargs=['--weak-external-drive'],
                              suppress_multidrive_messages=True,
                              tester=tester)

def runtest():
  for arch in ['xs2']:
    do_test(arch)
