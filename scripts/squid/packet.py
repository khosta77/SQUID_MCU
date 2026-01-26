from .protocol import PROTOCOL_STX, PROTOCOL_MIN_PACKET_SIZE, PROTOCOL_MAX_PACKET_SIZE


def calculate_xor(data: bytes) -> int:
    result = 0
    for b in data:
        result ^= b
    return result


class Packet:
    def __init__(self, command: int, data: bytes = b""):
        self.command = command
        self.data = data

    def to_bytes(self) -> bytes:
        total_length = PROTOCOL_MIN_PACKET_SIZE + len(self.data)
        length_h = (total_length >> 8) & 0xFF
        length_l = total_length & 0xFF

        payload = bytes([length_h, length_l, self.command]) + self.data
        xor_value = calculate_xor(payload)

        return bytes([PROTOCOL_STX]) + payload + bytes([xor_value])

    @classmethod
    def from_bytes(cls, data: bytes) -> "Packet":
        if len(data) < PROTOCOL_MIN_PACKET_SIZE:
            raise ValueError(f"Packet too short: {len(data)} bytes")

        if data[0] != PROTOCOL_STX:
            raise ValueError(f"Invalid STX: 0x{data[0]:02X}")

        length = (data[1] << 8) | data[2]
        if length != len(data):
            raise ValueError(f"Length mismatch: expected {length}, got {len(data)}")

        if length > PROTOCOL_MAX_PACKET_SIZE:
            raise ValueError(f"Packet too long: {length} bytes")

        command = data[3]
        payload_data = data[4:-1] if length > PROTOCOL_MIN_PACKET_SIZE else b""
        received_xor = data[-1]

        calculated_xor = calculate_xor(data[1:-1])
        if calculated_xor != received_xor:
            raise ValueError(f"XOR mismatch: expected 0x{calculated_xor:02X}, got 0x{received_xor:02X}")

        return cls(command=command, data=payload_data)
