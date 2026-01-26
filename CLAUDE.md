# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Firmware for STM32F407VG microcontroller controlling up to 10 stepper motors in synchronous and asynchronous modes. Uses pure CMSIS (no HAL/LL libraries).

**Target:** STM32F407VG (ARM Cortex-M4, 168 MHz, 1MB Flash, 192KB SRAM)

## Documentation

- [docs/COMMAND.md](docs/COMMAND.md) - Protocol commands and packet format
- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) - System architecture and diagrams
- [docs/FILETREE.md](docs/FILETREE.md) - File structure and modification guide
- [ABOUT.md](ABOUT.md) - System overview

## Build Commands

```bash
# Full build (generates main.elf, main.hex, main.bin)
make all

# Clean build artifacts
make clean

# Flash via ST-Link
st-flash write main.bin 0x08000000
```

**Toolchain:** arm-none-eabi-gcc, arm-none-eabi-g++

## Python CLI

```bash
# Install dependencies
poetry install

# Commands
poetry run python scripts/cli.py version
poetry run python scripts/cli.py status
poetry run python scripts/cli.py stop
poetry run python scripts/cli.py move -m 1 -s 5000

# Run unit tests
poetry run pytest tests/test_packet.py tests/test_motor.py -v

# Run integration tests (requires MCU)
poetry run pytest tests/test_integration.py -v --port /dev/tty.usbserial-*
```

## Architecture

### Hardware Interface

```
PC (Client) <--USB--> FTDI FT232RL <--UART4--> STM32F407VG <--USART2--> Motor Drivers (1-10)
```

### GPIO Pinout
- **PA0-PA1:** UART4 (PC communication)
- **PA2-PA3:** USART2 (motor drivers)
- **PB0-PB9:** KEY1-10 (data transmission enable)
- **PC0-PC9:** EN1-10 (driver power enable)
- **PD0-PD9:** SELECT1-10 (driver selection)
- **PE0-PE9:** STATUS1-10 (motor status interrupts)
- **PE10-PE15:** ENDSTOP1-6 (emergency stop)

### DMA Channels
- **DMA1_Stream2:** UART4_RX (commands from PC)
- **DMA1_Stream6:** USART2_TX (data to drivers)

### Code Structure

| File | Purpose |
|------|---------|
| `main.cpp` | Entry point, DMA/SysTick handlers, system init |
| `motor_controller.cpp/hpp` | Motor control logic, command processing |
| `motor_simulator.cpp/hpp` | Motor simulation with SysTick timer |
| `motor_settings.cpp/hpp` | MotorSettings class (16 bytes per motor) |
| `protocol.cpp/hpp` | Packet parser state machine |
| `constants.cpp/hpp` | Protocol constants, global state variables |
| `serial.cpp/hpp` | UART/USART initialization, packet sending |
| `gpio.cpp/hpp` | GPIO initialization |

### Protocol (Packet-based)

**Packet format:** `[STX=0x02] [Length_H] [Length_L] [Cmd] [Data...] [XOR]`

Commands:
- `0x01` - VERSION request
- `0x02` - STATUS request
- `0x03` - STOP all motors
- `0x10` - SYNC_MOVE (synchronous)
- `0x11` - ASYNC_MOVE (asynchronous)

Motor data: 16 bytes per motor (number, acceleration, maxSpeed, steps - all uint32_t little-endian)

See [docs/COMMAND.md](docs/COMMAND.md) for full protocol specification.

## Key Implementation Details

- All peripheral access via direct CMSIS register manipulation
- SysTick timer (1ms) for motor simulation
- PacketParser state machine for incoming data
- Interrupts: EXTI0-9 for motor status, EXTI10-15 for emergency stop (highest priority)
- Global state flags (`volatile`) track motor completion and emergency conditions
- DMA used for command reception
