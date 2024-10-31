# Copyright 2014-2024 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
from pathlib import Path
import Pyxsim
import pytest
import json
from i2c_slave_checker import I2CSlaveChecker

test_name = "i2c_slave_test"

@pytest.mark.parametrize("arch", ["xs2", "xs3"])
@pytest.mark.parametrize("speed", [400, 100, 10])
def test_basic_slave(capfd, request, nightly, speed, arch):
    cwd = Path(request.fspath).parent
    binary = f'{cwd}/{test_name}/bin/{arch}/{test_name}_{arch}.xe'

    assert Path(binary).exists(), f"Cannot find {binary}"

    checker = I2CSlaveChecker("tile[0]:XS1_PORT_1A",
                              "tile[0]:XS1_PORT_1B",
                              tsequence =
                              [("w", 0x3c, [0x33, 0x44, 0x3]),
                               ("r", 0x3c, 3),
                               ("w", 0x3c, [0x99]),
                               ("w", 0x44, [0x33]),
                               ("r", 0x3c, 1),
                               ("w", 0x3c, [0x22, 0xff])],
                               speed = speed)

    tester = Pyxsim.testers.AssertiveComparisonTester(
        f'{cwd}/expected/basic_slave_test.expect',
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

