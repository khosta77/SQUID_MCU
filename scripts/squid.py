#!/usr/bin/env python3
from __future__ import annotations

import glob
import sys
from typing import Optional

import click
import serial
from serial.tools import list_ports


CMD_VERSION = 0x20
FIRMWARE_VERSION_EXPECTED = 0x10


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


def open_serial(port: str, baudrate: int = 115200, timeout: float = 2.0) -> serial.Serial:
    return serial.Serial(
        port=port,
        baudrate=baudrate,
        bytesize=serial.EIGHTBITS,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        timeout=timeout,
    )


def send_version_command(ser: serial.Serial) -> Optional[int]:
    ser.reset_input_buffer()
    ser.write(bytes([CMD_VERSION]))
    response = ser.read(1)
    if response:
        return response[0]
    return None


@click.group()
@click.option("--port", "-p", default=None, help="Serial port (auto-detect if not specified)")
@click.pass_context
def cli(ctx: click.Context, port: Optional[str]) -> None:
    ctx.ensure_object(dict)
    if port is None:
        port = find_ftdi_port()
        if port:
            click.echo(f"Auto-detected port: {port}")
        else:
            click.echo("Error: No FTDI device found. Use --port to specify manually.", err=True)
            sys.exit(1)
    ctx.obj["port"] = port


@cli.command()
@click.pass_context
def version(ctx: click.Context) -> None:
    port = ctx.obj["port"]
    try:
        with open_serial(port) as ser:
            response = send_version_command(ser)
            if response is None:
                click.echo("Error: No response from device (timeout)", err=True)
                sys.exit(1)

            click.echo(f"Response: 0x{response:02X}")
            major = (response >> 4) & 0x0F
            minor = response & 0x0F
            click.echo(f"Firmware version: {major}.{minor}")

            if response == FIRMWARE_VERSION_EXPECTED:
                click.echo("Status: OK")
            else:
                click.echo(f"Status: WARNING (expected 0x{FIRMWARE_VERSION_EXPECTED:02X})")
    except serial.SerialException as e:
        click.echo(f"Serial error: {e}", err=True)
        sys.exit(1)


@cli.command()
@click.argument("byte", type=str)
@click.pass_context
def send(ctx: click.Context, byte: str) -> None:
    port = ctx.obj["port"]
    try:
        value = int(byte, 0)
        if not 0 <= value <= 255:
            raise ValueError("Byte must be 0-255")
    except ValueError as e:
        click.echo(f"Invalid byte value: {e}", err=True)
        sys.exit(1)

    try:
        with open_serial(port) as ser:
            ser.reset_input_buffer()
            ser.write(bytes([value]))
            click.echo(f"Sent: 0x{value:02X}")

            response = ser.read(1)
            if response:
                click.echo(f"Response: 0x{response[0]:02X}")
            else:
                click.echo("No response (timeout)")
    except serial.SerialException as e:
        click.echo(f"Serial error: {e}", err=True)
        sys.exit(1)


@cli.command()
@click.pass_context
def ports(ctx: click.Context) -> None:
    click.echo("Available serial ports:")
    for port in list_ports.comports():
        click.echo(f"  {port.device}: {port.description}")


def main() -> None:
    cli()


if __name__ == "__main__":
    main()
