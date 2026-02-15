#!/usr/bin/env python3
import asyncio
import glob
import sys
import time
from pathlib import Path
from typing import Optional

import click
from serial.tools import list_ports

sys.path.insert(0, str(Path(__file__).parent))

from squid import SquidClient, MotorParams, SquidError


def find_ftdi_port() -> Optional[str]:
    patterns = [
        "/dev/tty.usbserial-*",
        "/dev/ttyUSB*",
        "/dev/cu.usbserial-*",
    ]
    for pattern in patterns:
        matches = glob.glob(pattern)
        if matches:
            return matches[0]

    for port in list_ports.comports():
        if "FTDI" in port.description or "FT232" in port.description:
            return port.device

    return None


def run_async(coro):
    return asyncio.run(coro)


@click.group()
@click.option("--port", "-p", default=None, help="Serial port (auto-detect if not specified)")
@click.option("--baudrate", "-b", default=115200, help="Baud rate")
@click.pass_context
def cli(ctx, port: Optional[str], baudrate: int):
    ctx.ensure_object(dict)
    if port is None:
        port = find_ftdi_port()
        if port:
            click.echo(f"Auto-detected port: {port}")
        else:
            click.echo("Error: No FTDI device found. Use --port to specify.", err=True)
            sys.exit(1)
    ctx.obj["port"] = port
    ctx.obj["baudrate"] = baudrate


@cli.command()
@click.pass_context
def version(ctx):
    async def _version():
        async with SquidClient(ctx.obj["port"], ctx.obj["baudrate"]) as client:
            ver = await client.get_version()
            click.echo(f"Firmware version: {ver}")

    try:
        run_async(_version())
    except SquidError as e:
        click.echo(f"Error: {e}", err=True)
        sys.exit(1)


@cli.command()
@click.pass_context
def status(ctx):
    async def _status():
        async with SquidClient(ctx.obj["port"], ctx.obj["baudrate"]) as client:
            active, completed, status_pins = await client.get_status()
            click.echo(f"Active motors:    0x{active:04X} (bin: {active:010b})")
            click.echo(f"Completed motors: 0x{completed:04X} (bin: {completed:010b})")
            click.echo(f"STATUS pins:      0x{status_pins:04X} (bin: {status_pins:010b})")

    try:
        run_async(_status())
    except SquidError as e:
        click.echo(f"Error: {e}", err=True)
        sys.exit(1)


@cli.command()
@click.pass_context
def stop(ctx):
    async def _stop():
        async with SquidClient(ctx.obj["port"], ctx.obj["baudrate"]) as client:
            result = await client.stop()
            if result:
                click.echo("Stop command sent successfully")
            else:
                click.echo("Stop command failed", err=True)

    try:
        run_async(_stop())
    except SquidError as e:
        click.echo(f"Error: {e}", err=True)
        sys.exit(1)


@cli.command()
@click.option("--motor", "-m", required=True, type=int, help="Motor number (1-10)")
@click.option("--steps", "-s", required=True, type=int, help="Number of steps")
@click.option("--speed", default=1000, type=int, help="Max speed")
@click.option("--accel", default=500, type=int, help="Acceleration")
@click.option("--async", "async_mode", is_flag=True, help="Use async mode")
@click.option("--timeout", "-t", default=300, type=float, help="Timeout in seconds")
@click.pass_context
def move(ctx, motor: int, steps: int, speed: int, accel: int, async_mode: bool, timeout: float):
    async def _move():
        async with SquidClient(ctx.obj["port"], ctx.obj["baudrate"]) as client:
            params = MotorParams(number=motor, acceleration=accel, max_speed=speed, steps=steps)
            t0 = time.perf_counter()
            if async_mode:
                result = await client.async_move([params], timeout=timeout)
            else:
                result = await client.sync_move([params], timeout=timeout)
            elapsed = time.perf_counter() - t0

            if result:
                click.echo(f"Move completed: motor {motor}, {steps} steps ({elapsed:.3f} s)")
            else:
                click.echo(f"Move failed ({elapsed:.3f} s)", err=True)

    try:
        run_async(_move())
    except SquidError as e:
        click.echo(f"Error: {e}", err=True)
        sys.exit(1)


@cli.command()
@click.argument("motors", nargs=-1, type=str)
@click.option("--async", "async_mode", is_flag=True, help="Use async mode")
@click.option("--timeout", "-t", default=300, type=float, help="Timeout in seconds")
@click.pass_context
def multi_move(ctx, motors: tuple, async_mode: bool, timeout: float):
    async def _multi_move():
        params_list = []
        for m in motors:
            parts = m.split(":")
            if len(parts) != 4:
                click.echo(f"Invalid format: {m}. Use motor:accel:speed:steps", err=True)
                return

            motor_num, accel, speed, steps = map(int, parts)
            params_list.append(MotorParams(number=motor_num, acceleration=accel, max_speed=speed, steps=steps))

        async with SquidClient(ctx.obj["port"], ctx.obj["baudrate"]) as client:
            t0 = time.perf_counter()
            if async_mode:
                result = await client.async_move(params_list, timeout=timeout)
            else:
                result = await client.sync_move(params_list, timeout=timeout)
            elapsed = time.perf_counter() - t0

            if result:
                click.echo(f"Multi-move completed: {len(params_list)} motors ({elapsed:.3f} s)")
            else:
                click.echo(f"Multi-move failed ({elapsed:.3f} s)", err=True)

    try:
        run_async(_multi_move())
    except SquidError as e:
        click.echo(f"Error: {e}", err=True)
        sys.exit(1)


if __name__ == "__main__":
    cli()
