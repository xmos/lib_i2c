# Copyright 2014-2024 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
from pathlib import Path
import Pyxsim
import pytest
import json
from i2c_master_checker import I2CMasterChecker

DEBUG = False
test_name = "i2c_master_test"

with open(Path(__file__).parent / f"{test_name}/test_params.json") as f:
    params = json.load(f)

@pytest.mark.parametrize("arch", ["xs2", "xs3"])
@pytest.mark.parametrize("dir", ["rx_tx"]) # only test the rx_tx config
@pytest.mark.parametrize("speed", [400, 100])
@pytest.mark.parametrize("stop", params['STOPS'])
def test_master_clock_stretch(capfd, request, nightly, dir, speed, stop, arch):
    cwd = Path(request.fspath).parent
    cfg = f"{dir}_{speed}_{stop}_{arch}"
    binary = f'{cwd}/{test_name}/bin/{cfg}/{test_name}_{cfg}.xe'

    assert Path(binary).exists(), f"Cannot find {binary}"

    if speed == 400:
        expected_speed = 160
    else:
        expected_speed = 100
    checker = I2CMasterChecker("tile[0]:XS1_PORT_1A",
                               "tile[0]:XS1_PORT_1B",
                               tx_data = [0x99, 0x3A, 0xff],
                               expected_speed=expected_speed,
                               clock_stretch=5000,
                               ack_sequence=[True, True, False, # Master write
                                             True, # Master read
                                             True, # Master read
                                             True, True, True, False, # Master write
                                             True, False], # Master write
                               original_speed = speed # Timing checks use the original speed that the I2C master is configured to run at
                               )

    tester = Pyxsim.testers.AssertiveComparisonTester(
        f'{cwd}/expected/master_test_{stop}.expect',
        regexp = True,
        ordered = True,
        suppress_multidrive_messages=True,
    )

    if DEBUG:
        with capfd.disabled():
            Pyxsim.run_on_simulator_(
                binary,
                tester = tester,
                do_xe_prebuild = False,
                simthreads=[checker],
                simargs=[
                    "--vcd-tracing",
                    f"-o i2c_trace.vcd -tile tile[0] -cycles -ports -ports-detailed -cores -instructions",
                    "--trace-to",
                    f"i2c_trace.txt",
                    '--weak-external-drive'
                ],
            )
    else:
        Pyxsim.run_on_simulator_(
            binary,
            tester = tester,
            do_xe_prebuild = False,
            simthreads = [checker],
            simargs=['--weak-external-drive'],
            capfd=capfd
            )
