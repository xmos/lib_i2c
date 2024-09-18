# Copyright 2014-2024 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
from pathlib import Path
import Pyxsim
import pytest
import json
from i2c_master_checker import I2CMasterChecker

test_name = "i2c_master_reg_test"

@pytest.mark.parametrize("arch", ["xs2", "xcoreai"])
def test_reg_ops_nack(capfd, request, nightly, arch):
    cwd = Path(request.fspath).parent
    binary = f'{cwd}/{test_name}/bin/{arch}/{test_name}_{arch}.xe'

    assert Path(binary).exists(), f"Cannot find {binary}"

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

    tester = Pyxsim.testers.AssertiveComparisonTester(
        f'{cwd}/expected/reg_ops_nack.expect',
        regexp = True,
        ordered = True,
        suppress_multidrive_messages=True,
    )

    Pyxsim.run_on_simulator_(
        binary,
        tester = tester,
        do_xe_prebuild = False,
        simthreads = [checker],
        simargs=['--weak-external-drive'],
        capfd=capfd
        )
