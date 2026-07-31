# Copyright 2026 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
from pathlib import Path

import Pyxsim
import pytest

from i2c_slave_checker import I2CSlaveChecker


test_name = "i2c_slave_test"


class I2CSlaveSclSpikeChecker(I2CSlaveChecker):
    def __init__(self, *args, spike_pulse_index, spike_width_ns=49,
                 **kwargs):
        super().__init__(*args, **kwargs)
        self._pulse_index = 0
        if isinstance(spike_pulse_index, int):
            spike_pulse_index = [spike_pulse_index]
        self._spike_pulse_indices = set(spike_pulse_index)
        self._spike_width = spike_width_ns * 1e6

    def maybe_spike(self, xsi):
        if self._pulse_index in self._spike_pulse_indices:
            self.inject_scl_high_spike(xsi)
        self._pulse_index += 1

    def inject_scl_high_spike(self, xsi):
        xsi.drive_port_pins(self._scl_port, 1)
        self.wait_until(xsi.get_time() + self._spike_width)
        xsi.drive_port_pins(self._scl_port, 0)

    def high_pulse(self, xsi):
        self.maybe_spike(xsi)
        super().high_pulse(xsi)

    def high_pulse_sample(self, xsi):
        self.maybe_spike(xsi)
        return super().high_pulse_sample(xsi)


def run_slave_scl_spike_filter_test(capfd, cwd, binary, spike_pulse_index):
    checker = I2CSlaveSclSpikeChecker(
        "tile[0]:XS1_PORT_1A",
        "tile[0]:XS1_PORT_1B",
        tsequence=[
            ("w", 0x3C, [0x33, 0x44, 0x03]),
            ("r", 0x3C, 3),
            ("w", 0x3C, [0x99]),
            ("w", 0x44, [0x33]),
            ("r", 0x3C, 1),
            ("w", 0x3C, [0x22, 0xFF]),
        ],
        speed=400,
        spike_pulse_index=spike_pulse_index,
    )

    tester = Pyxsim.testers.AssertiveComparisonTester(
        f"{cwd}/expected/basic_slave_test.expect",
        regexp=True,
        ordered=True,
        suppress_multidrive_messages=True,
    )

    Pyxsim.run_on_simulator_(
        binary,
        tester=tester,
        do_xe_prebuild=False,
        simthreads=[checker],
        simargs=["--weak-external-drive"],
        capfd=capfd,
    )


@pytest.mark.parametrize("arch", ["xs3"])
@pytest.mark.parametrize(
    "spike_name, spike_pulse_index",
    [
        ("address_first_bit", 0),
        ("address_mid_bit", 4),
        ("address_ack", 8),
        ("write_data_first_bit", 9),
        ("write_data_ack", 17),
        ("read_data_first_bit", 45),
        ("read_data_ack", 53),
        ("read_next_byte_first_bit", 54),
        ("read_next_byte_ack", 62),
        ("read_final_nack", 71),
    ],
)
def test_slave_scl_spike_filter(capfd, request, arch, spike_name,
                                spike_pulse_index):
    cwd = Path(request.fspath).parent
    binary = f"{cwd}/{test_name}/bin/{arch}/{test_name}_{arch}.xe"

    assert Path(binary).exists(), f"Cannot find {binary}"

    run_slave_scl_spike_filter_test(capfd, cwd, binary, spike_pulse_index)


@pytest.mark.parametrize("arch", ["xs3"])
def test_slave_scl_spike_filter_multiple_glitches(capfd, request, arch):
    cwd = Path(request.fspath).parent
    binary = f"{cwd}/{test_name}/bin/{arch}/{test_name}_{arch}.xe"

    assert Path(binary).exists(), f"Cannot find {binary}"

    run_slave_scl_spike_filter_test(
        capfd,
        cwd,
        binary,
        [0, 8, 17, 45, 53, 71],
    )
