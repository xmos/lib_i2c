# Copyright 2014-2024 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
from pathlib import Path
import Pyxsim
import pytest
import json
from i2c_master_checker import I2CMasterChecker

test_name = "i2c_test_locks"

def test_reg_ops(capfd, request, nightly):
    cwd = Path(request.fspath).parent
    arch = "xcoreai"
    binary = f'{cwd}/{test_name}/bin/{test_name}.xe'

    assert Path(binary).exists(), f"Cannot find {binary}"

    speed = 400
    checker = I2CMasterChecker("tile[0]:XS1_PORT_1A",
                               "tile[0]:XS1_PORT_1B",
                               expected_speed=speed)

    tester = Pyxsim.testers.AssertiveComparisonTester(
        f'{cwd}/lock_test.expect',
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

