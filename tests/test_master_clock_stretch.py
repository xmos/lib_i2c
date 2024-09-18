# Copyright 2014-2024 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
from pathlib import Path
import Pyxsim
import pytest
import json
from i2c_master_checker import I2CMasterChecker

test_name = "i2c_master_test"

with open(Path(__file__).parent / f"{test_name}/test_params.json") as f:
    params = json.load(f)

@pytest.mark.parametrize("arch", ["xs2", "xcoreai"])
@pytest.mark.parametrize("dir", ["rx_tx"]) # only test the rx_tx config
@pytest.mark.parametrize("speed", [400]) # only test speed = 400
@pytest.mark.parametrize("stop", params['STOPS'])
def test_master_clock_stretch(capfd, request, nightly, dir, speed, stop, arch):
    cwd = Path(request.fspath).parent
    cfg = f"{dir}_{speed}_{stop}_{arch}"
    binary = f'{cwd}/{test_name}/bin/{cfg}/{test_name}_{cfg}.xe'

    assert Path(binary).exists(), f"Cannot find {binary}"

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

    tester = Pyxsim.testers.AssertiveComparisonTester(
        f'{cwd}/expected/master_test_{stop}.expect',
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
