#!/usr/bin/env python3
import asyncio
from squid_client import SquidClient, MotorParams

async def main():
    async with SquidClient("/dev/tty.usbserial-A50285BI", 115200) as client:
        params = [
            MotorParams(number=1, acceleration=500, max_speed=1000, steps=1000),
            MotorParams(number=2, acceleration=500, max_speed=1000, steps=2000),
            MotorParams(number=3, acceleration=500, max_speed=1000, steps=3000),
        ]
        result = await client.sync_move(params, timeout=10.0)
        print(f"Result: {result}")

if __name__ == "__main__":
    asyncio.run(main())
