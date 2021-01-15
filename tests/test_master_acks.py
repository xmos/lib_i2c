# Copyright (c) 2014-2021, XMOS Ltd, All rights reserved
import xmostest
from i2c_master_checker import I2CMasterChecker

def do_master_test(arch, stop):
    resources = xmostest.request_resource("xsim")

    binary = 'i2c_master_test/bin/tx_only_%(stop)s_%(arch)s/i2c_master_test_tx_only_%(stop)s_%(arch)s.xe' % {
      'stop' : stop,
      'arch' : arch,
    }

    checker = I2CMasterChecker("tile[0]:XS1_PORT_1A",
                               "tile[0]:XS1_PORT_1B",
                               tx_data = [0x99, 0x3A, 0xff],
                               expected_speed = 400,
                               ack_sequence=[True, True, True,
                                             True, True, False,
                                             False, True])

    tester = xmostest.ComparisonTester(open('ack_test_%s.expect' % stop),
                                      'lib_i2c', 'i2c_master_sim_tests',
                                      'ack_test',
                                      {'speed':400, 'arch' : arch, 'stop' : stop},
                                      regexp=True)

    xmostest.run_on_simulator(resources['xsim'], binary,
                              simthreads = [checker],
                              simargs=['--weak-external-drive'],
                              suppress_multidrive_messages = True,
                              tester = tester)

def runtest():
  for arch in ['xs1', 'xs2', 'xcoreai']:
    for stop in ['stop', 'no_stop']:
        do_master_test(arch, stop)
