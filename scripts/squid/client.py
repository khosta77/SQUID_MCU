from typing import Optional

from .transport import AsyncSerialTransport
from .packet import Packet
from .protocol import Command, Response, ErrorCode
from .motor import MotorParams
from .errors import ProtocolError


class SquidClient:
    def __init__(self, port: str, baudrate: int = 115200):
        self._transport = AsyncSerialTransport(port, baudrate)

    async def connect(self) -> None:
        await self._transport.connect()

    async def disconnect(self) -> None:
        await self._transport.disconnect()

    async def __aenter__(self) -> "SquidClient":
        await self.connect()
        return self

    async def __aexit__(self, exc_type, exc_val, exc_tb) -> None:
        await self.disconnect()

    async def _send_and_receive(
        self, command: int, data: bytes = b"", timeout: float = 5.0
    ) -> Packet:
        packet = Packet(command, data)
        await self._transport.send_packet(packet)
        response = await self._transport.receive_packet(timeout)

        if response.command == Response.ERROR:
            error_code = response.data[0] if response.data else 0
            raise ProtocolError(error_code, self._error_message(error_code))

        return response

    @staticmethod
    def _error_message(code: int) -> str:
        messages = {
            ErrorCode.INVALID_COMMAND: "Invalid command",
            ErrorCode.INVALID_PACKET_LENGTH: "Invalid packet length",
            ErrorCode.XOR_CHECKSUM_ERROR: "XOR checksum error",
            ErrorCode.INVALID_MOTOR_COUNT: "Invalid motor count",
            ErrorCode.MOTOR_PARAM_ERROR: "Motor parameter validation error",
            ErrorCode.EMERGENCY_STOP: "Emergency stop triggered",
            ErrorCode.TIMEOUT: "Timeout",
        }
        return messages.get(code, "Unknown error")

    async def get_version(self) -> str:
        response = await self._send_and_receive(Command.VERSION)
        if response.data:
            version = response.data[0]
            major = (version >> 4) & 0x0F
            minor = version & 0x0F
            return f"{major}.{minor}"
        return "unknown"

    async def get_status(self) -> tuple[int, int, int]:
        response = await self._send_and_receive(Command.STATUS)
        if len(response.data) >= 6:
            active = response.data[0] | (response.data[1] << 8)
            completed = response.data[2] | (response.data[3] << 8)
            status_pins = response.data[4] | (response.data[5] << 8)
        else:
            active = response.data[0] if len(response.data) > 0 else 0
            completed = response.data[1] if len(response.data) > 1 else 0
            status_pins = 0
        return active, completed, status_pins

    async def stop(self) -> bool:
        response = await self._send_and_receive(Command.STOP)
        return response.data[0] == 0x00 if response.data else False

    async def sync_move(
        self, motors: list[MotorParams], timeout: float = 300.0
    ) -> bool:
        data = b"".join(m.to_bytes() for m in motors)
        response = await self._send_and_receive(Command.SYNC_MOVE, data, timeout)
        return response.data[0] == 0x00 if response.data else False

    async def async_move(
        self, motors: list[MotorParams], timeout: float = 300.0
    ) -> bool:
        data = b"".join(m.to_bytes() for m in motors)
        response = await self._send_and_receive(Command.ASYNC_MOVE, data, timeout)
        return response.data[0] == 0x00 if response.data else False
