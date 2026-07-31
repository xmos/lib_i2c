# Copyright 2026 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
from pathlib import Path

import Pyxsim
import pytest

from i2c_slave_checker import I2CSlaveChecker


@pytest.mark.parametrize("arch", ["xs2", "xs3"])
def test_slave_no_clock_stretch(capfd, request, arch):
    cwd = Path(request.fspath).parent
    config = f"no_stretch_{arch}"
    binary = cwd / "i2c_slave_test" / "bin" / config / f"i2c_slave_test_{config}.xe"

    assert binary.exists(), f"Cannot find {binary}"

    checker = I2CSlaveChecker(
        "tile[0]:XS1_PORT_1A",
        "tile[0]:XS1_PORT_1B",
        tsequence=[
            ("r", 0x3C, 1),
            ("w", 0x3C, [0x33, 0xFF]),
        ],
        speed=10,
        allow_clock_stretch=False,
    )

    tester = Pyxsim.testers.AssertiveComparisonTester(
        f"{cwd}/expected/no_clock_stretch.expect",
        regexp=True,
        ordered=True,
        suppress_multidrive_messages=True,
    )

    Pyxsim.run_on_simulator_(
        str(binary),
        tester=tester,
        do_xe_prebuild=False,
        simthreads=[checker],
        simargs=["--weak-external-drive"],
        capfd=capfd,
    )
