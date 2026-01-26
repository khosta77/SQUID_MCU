from dataclasses import dataclass
import struct


@dataclass
class MotorParams:
    number: int
    acceleration: int
    max_speed: int
    steps: int

    def to_bytes(self) -> bytes:
        return struct.pack("<IIII", self.number, self.acceleration, self.max_speed, self.steps)

    @classmethod
    def from_bytes(cls, data: bytes) -> "MotorParams":
        number, acceleration, max_speed, steps = struct.unpack("<IIII", data[:16])
        return cls(number=number, acceleration=acceleration, max_speed=max_speed, steps=steps)
