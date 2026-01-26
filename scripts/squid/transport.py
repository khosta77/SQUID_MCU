import asyncio
from typing import Optional

import aioserial

from .packet import Packet
from .protocol import PROTOCOL_STX, PROTOCOL_MIN_PACKET_SIZE, PROTOCOL_MAX_PACKET_SIZE
from .errors import TimeoutError, ChecksumError


class AsyncSerialTransport:
    def __init__(self, port: str, baudrate: int = 115200):
        self._port = port
        self._baudrate = baudrate
        self._serial: Optional[aioserial.AioSerial] = None
        self._lock = asyncio.Lock()

    async def connect(self) -> None:
        self._serial = aioserial.AioSerial(
            port=self._port,
            baudrate=self._baudrate,
            timeout=0.1,
        )

    async def disconnect(self) -> None:
        if self._serial and self._serial.is_open:
            self._serial.close()
        self._serial = None

    async def send_packet(self, packet: Packet) -> None:
        if not self._serial:
            raise RuntimeError("Not connected")

        async with self._lock:
            data = packet.to_bytes()
            await self._serial.write_async(data)

    async def receive_packet(self, timeout: float = 5.0) -> Packet:
        if not self._serial:
            raise RuntimeError("Not connected")

        async with self._lock:
            buffer = bytearray()
            expected_length = 0
            start_time = asyncio.get_event_loop().time()

            while True:
                elapsed = asyncio.get_event_loop().time() - start_time
                if elapsed > timeout:
                    raise TimeoutError(f"Timeout waiting for response ({timeout}s)")

                try:
                    byte = await asyncio.wait_for(
                        self._serial.read_async(1),
                        timeout=min(0.1, timeout - elapsed),
                    )
                except asyncio.TimeoutError:
                    continue

                if not byte:
                    continue

                if len(buffer) == 0 and byte[0] != PROTOCOL_STX:
                    continue

                buffer.extend(byte)

                if len(buffer) == 3:
                    expected_length = (buffer[1] << 8) | buffer[2]
                    if expected_length < PROTOCOL_MIN_PACKET_SIZE or expected_length > PROTOCOL_MAX_PACKET_SIZE:
                        buffer.clear()
                        continue

                if expected_length > 0 and len(buffer) >= expected_length:
                    try:
                        return Packet.from_bytes(bytes(buffer))
                    except ValueError as e:
                        if "XOR" in str(e):
                            raise ChecksumError(str(e)) from e
                        buffer.clear()
                        continue

    async def __aenter__(self) -> "AsyncSerialTransport":
        await self.connect()
        return self

    async def __aexit__(self, exc_type, exc_val, exc_tb) -> None:
        await self.disconnect()
