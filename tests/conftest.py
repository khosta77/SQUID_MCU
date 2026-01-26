import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).parent.parent / "scripts"))

from squid import SquidClient


def pytest_addoption(parser):
    parser.addoption(
        "--port",
        action="store",
        default=None,
        help="Serial port for integration tests",
    )
    parser.addoption(
        "--baudrate",
        action="store",
        default=115200,
        type=int,
        help="Baud rate for serial communication",
    )


@pytest.fixture
def serial_port(request):
    return request.config.getoption("--port")


@pytest.fixture
def baudrate(request):
    return request.config.getoption("--baudrate")


@pytest.fixture
async def squid_client(serial_port, baudrate):
    if serial_port is None:
        pytest.skip("No serial port specified (use --port)")

    client = SquidClient(serial_port, baudrate)
    await client.connect()
    yield client
    await client.disconnect()
