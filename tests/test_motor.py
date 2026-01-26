import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).parent.parent / "scripts"))

from squid.motor import MotorParams


class TestMotorParamsToBytes:
    def test_basic_params(self):
        params = MotorParams(number=1, acceleration=500, max_speed=1000, steps=5000)
        data = params.to_bytes()

        assert len(data) == 16
        assert data[0:4] == b"\x01\x00\x00\x00"
        assert data[4:8] == b"\xf4\x01\x00\x00"
        assert data[8:12] == b"\xe8\x03\x00\x00"
        assert data[12:16] == b"\x88\x13\x00\x00"

    def test_large_values(self):
        params = MotorParams(number=10, acceleration=0xFFFFFFFF, max_speed=0xFFFFFFFF, steps=0xFFFFFFFF)
        data = params.to_bytes()

        assert len(data) == 16


class TestMotorParamsFromBytes:
    def test_basic_params(self):
        data = b"\x01\x00\x00\x00\xf4\x01\x00\x00\xe8\x03\x00\x00\x88\x13\x00\x00"
        params = MotorParams.from_bytes(data)

        assert params.number == 1
        assert params.acceleration == 500
        assert params.max_speed == 1000
        assert params.steps == 5000


class TestMotorParamsRoundtrip:
    def test_roundtrip(self):
        original = MotorParams(number=5, acceleration=1234, max_speed=5678, steps=90123)
        data = original.to_bytes()
        restored = MotorParams.from_bytes(data)

        assert restored.number == original.number
        assert restored.acceleration == original.acceleration
        assert restored.max_speed == original.max_speed
        assert restored.steps == original.steps
