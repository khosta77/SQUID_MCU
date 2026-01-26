import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).parent.parent / "scripts"))

from squid import SquidClient, MotorParams, SquidError, ProtocolError
from squid.protocol import ErrorCode


pytestmark = pytest.mark.asyncio


class TestVersionCommand:
    async def test_get_version(self, squid_client):
        version = await squid_client.get_version()
        assert version == "1.0"


class TestStatusCommand:
    async def test_get_status_idle(self, squid_client):
        active, completed = await squid_client.get_status()
        assert isinstance(active, int)
        assert isinstance(completed, int)


class TestStopCommand:
    async def test_stop_success(self, squid_client):
        result = await squid_client.stop()
        assert result is True


class TestSyncMoveCommand:
    async def test_single_motor_move(self, squid_client):
        params = MotorParams(number=1, acceleration=500, max_speed=1000, steps=2000)
        result = await squid_client.sync_move([params], timeout=10.0)
        assert result is True

    async def test_multiple_motors_move(self, squid_client):
        params_list = [
            MotorParams(number=1, acceleration=500, max_speed=1000, steps=1000),
            MotorParams(number=2, acceleration=500, max_speed=1000, steps=2000),
        ]
        result = await squid_client.sync_move(params_list, timeout=10.0)
        assert result is True


class TestAsyncMoveCommand:
    async def test_single_motor_async(self, squid_client):
        params = MotorParams(number=1, acceleration=500, max_speed=1000, steps=5000)
        result = await squid_client.async_move([params], timeout=5.0)
        assert result is True

    async def test_check_status_after_async(self, squid_client):
        params = MotorParams(number=1, acceleration=500, max_speed=1000, steps=5000)
        await squid_client.async_move([params], timeout=5.0)

        active, completed = await squid_client.get_status()
        assert active >= 0
        assert completed >= 0


class TestErrorHandling:
    async def test_invalid_motor_count(self, squid_client):
        params_list = [
            MotorParams(number=i, acceleration=500, max_speed=1000, steps=1000)
            for i in range(1, 15)
        ]
        with pytest.raises(ProtocolError) as exc_info:
            await squid_client.sync_move(params_list, timeout=5.0)

        assert exc_info.value.error_code == ErrorCode.INVALID_MOTOR_COUNT
