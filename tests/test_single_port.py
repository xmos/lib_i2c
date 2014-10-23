#!/usr/bin/env python
import xmostest
from i2c_master_checker import I2CMasterChecker
import os

def do_sp_test(speed):
    resources = xmostest.request_resource("xsim")

    xmostest.build('i2c_sp_test')

    binary = 'i2c_sp_test/bin/%(speed)d/i2c_sp_test_%(speed)d.xe' % {'speed':speed}

    checker = I2CMasterChecker("tile[0]:XS1_PORT_8A.1",
                               "tile[0]:XS1_PORT_8A.3",
                               tx_data = [0x99, 0x3A, 0xff],
                               expected_speed = speed)

    tester = xmostest.pass_if_matches(open('single_port_test.expect'),
                                     'lib_i2c', 'i2c_master_sim_tests',
                                     'single_port_test', {'speed':speed},
                                     regexp=True)

    xmostest.run_on_simulator(resources['xsim'], binary,
                              simthreads = [checker],
                              simargs=['--weak-external-drive'],
                              suppress_multidrive_messages = True,
                              tester = tester)


def runtest():
    do_sp_test(400)
    do_sp_test(100)
    if xmostest.get_testrun_type() != 'smoke':
        do_sp_test(10)
    else:
        xmostest.note_skipped_tests()
