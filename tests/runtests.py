import xmostest
from i2c_master_checker import i2c_master_checker

xmostest.init()

xmostest.register_group("lib_i2c",
                        "i2c_master_sim_tests",
                        "I2C master simulator tests",
"""
Tests are performed by running the I2C library connected to a
simulator model (written as a python plugin to xsim). The simulator
model checks that the signals comply to the I2C specification and checks the
protocol speed of the transactions. Tests are run to test the following
features:

   * Transmission of packets
   * Reception of packets

The tests are run with transactions of varying number of bytes and with rx and
tx transactions interleaved. The tests are run at speeds of 10, 100 and 400
Kbps.
""")

resources = xmostest.request_resource("xsim")

xmostest.build('i2c_master_test')

binary = 'i2c_master_test/bin/i2c_master_test.xe'

test = ('lib_i2c', 'i2c_master_sim_tests', 'basic_test', {'speed':400})

xmostest.run_on_simulator(resources['xsim'], binary,
                          simthreads = [ (i2c_master_checker,
                                          "tile[0]:XS1_PORT_1A",
                                          "tile[0]:XS1_PORT_1B",
                                          [0x99, 0x3A, 0xff],
                                          400)],
                          simargs=['--weak-external-drive'],
                          suppress_multidrive_messages = True,
                          tester = \
                             xmostest.pass_if_matches_file('master_400.expect',
                                                           *test,
                                                           regexp=True))

xmostest.finish()
