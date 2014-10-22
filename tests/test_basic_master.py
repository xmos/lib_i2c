#!/usr/bin/env python
import xmostest
from i2c_master_checker import I2CMasterChecker
import os


def do_master_test(speed):
    resources = xmostest.request_resource("xsim")

    xmostest.build('i2c_master_test')

    binary = 'i2c_master_test/bin/rx_tx_%(speed)d/i2c_master_test_rx_tx_%(speed)d.xe' % {'speed':speed}

    checker = I2CMasterChecker("tile[0]:XS1_PORT_1A",
                               "tile[0]:XS1_PORT_1B",
                               tx_data = [0x99, 0x3A, 0xff],
                               expected_speed = speed)

    tester = xmostest.pass_if_matches(open('master_test.expect'),
                                     'lib_i2c', 'i2c_master_sim_tests',
                                     'basic_test', {'speed':speed},
                                     regexp=True)

    xmostest.run_on_simulator(resources['xsim'], binary,
                              simthreads = [checker],
                              simargs=['--weak-external-drive'],
                              suppress_multidrive_messages = True,
                              tester = tester)

def runtest():
    do_master_test(400)
    do_master_test(100)
    if xmostest.get_testrun_type() != 'smoke':
        do_master_test(10)
    else:
        xmostest.note_skipped_tests()
