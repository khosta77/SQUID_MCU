import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).parent.parent / "scripts"))

from squid.packet import Packet, calculate_xor
from squid.protocol import PROTOCOL_STX, Command, Response


class TestCalculateXor:
    def test_empty_bytes(self):
        assert calculate_xor(b"") == 0

    def test_single_byte(self):
        assert calculate_xor(b"\x42") == 0x42

    def test_multiple_bytes(self):
        assert calculate_xor(b"\x01\x02\x03") == 0x01 ^ 0x02 ^ 0x03

    def test_xor_identity(self):
        assert calculate_xor(b"\xFF\xFF") == 0


class TestPacketToBytes:
    def test_version_command(self):
        packet = Packet(Command.VERSION)
        data = packet.to_bytes()

        assert data[0] == PROTOCOL_STX
        assert data[1] == 0x00
        assert data[2] == 0x05
        assert data[3] == Command.VERSION
        expected_xor = 0x00 ^ 0x05 ^ Command.VERSION
        assert data[4] == expected_xor

    def test_packet_with_data(self):
        packet = Packet(Command.SYNC_MOVE, b"\x01\x02\x03\x04")
        data = packet.to_bytes()

        assert data[0] == PROTOCOL_STX
        total_len = 5 + 4
        assert data[1] == (total_len >> 8) & 0xFF
        assert data[2] == total_len & 0xFF
        assert data[3] == Command.SYNC_MOVE
        assert data[4:8] == b"\x01\x02\x03\x04"


class TestPacketFromBytes:
    def test_valid_version_response(self):
        raw = bytes([PROTOCOL_STX, 0x00, 0x06, Response.VERSION, 0x10])
        xor_val = 0x00 ^ 0x06 ^ Response.VERSION ^ 0x10
        raw += bytes([xor_val])

        packet = Packet.from_bytes(raw)

        assert packet.command == Response.VERSION
        assert packet.data == bytes([0x10])

    def test_invalid_stx(self):
        raw = bytes([0x00, 0x00, 0x05, 0x01, 0x04])
        with pytest.raises(ValueError, match="Invalid STX"):
            Packet.from_bytes(raw)

    def test_invalid_xor(self):
        raw = bytes([PROTOCOL_STX, 0x00, 0x05, 0x01, 0xFF])
        with pytest.raises(ValueError, match="XOR mismatch"):
            Packet.from_bytes(raw)

    def test_packet_too_short(self):
        raw = bytes([PROTOCOL_STX, 0x00, 0x05])
        with pytest.raises(ValueError, match="too short"):
            Packet.from_bytes(raw)

    def test_length_mismatch(self):
        raw = bytes([PROTOCOL_STX, 0x00, 0x10, 0x01, 0x00, 0x00])
        with pytest.raises(ValueError, match="Length mismatch"):
            Packet.from_bytes(raw)


class TestPacketRoundtrip:
    def test_version_roundtrip(self):
        original = Packet(Command.VERSION)
        raw = original.to_bytes()
        restored = Packet.from_bytes(raw)

        assert restored.command == original.command
        assert restored.data == original.data

    def test_data_roundtrip(self):
        original = Packet(Command.SYNC_MOVE, b"\x01\x00\x00\x00" * 4)
        raw = original.to_bytes()
        restored = Packet.from_bytes(raw)

        assert restored.command == original.command
        assert restored.data == original.data
